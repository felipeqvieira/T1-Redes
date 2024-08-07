#include "../lib/utils.h"
#include "../lib/connection.h"


/* Auxiliary Functions */
uint8_t crc8_calc(packet_t *packet);
double diff_time(clock_t start, clock_t end);
int start_marker_verification(packet_t *p);
int packet_verification(uint8_t size, uint8_t sequence, uint8_t type);
int crc8_verification(packet_t *p);
void shift_bits(packet_t *packet);
void unshift_bits(packet_t* p);


/* *** Main Functions *** */

/* Create a socket and make connection with the given interface in promiscuous mode */
int create_socket(char *device)
{
  int sock;
  struct sockaddr_ll adress;
  struct ifreq ir;
  struct packet_mreq mr;

  /* Create a Socket */
  sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock == -1)
  {
    fprintf(stderr,"ERROR: you have to execute with sudo!\n");
    return ERR_PERMISSION;
  }

  /* Interface */
  memset(&ir, 0, sizeof(struct ifreq));
  memcpy(ir.ifr_name, device, sizeof(&device));
  if (ioctl(sock, SIOCGIFINDEX, &ir) == -1)
  {
    printf("ERROR: device not found, please try again with a valid device!\n");
    return ERR_INTERFACE;
  }

  /* Connecting socket to interface (bind) */
  memset(&adress, 0, sizeof(adress)); 	
  adress.sll_protocol = htons(ETH_P_ALL);
  adress.sll_family = AF_PACKET;
  adress.sll_ifindex = ir.ifr_ifindex;
  if (bind(sock, (struct sockaddr *)&adress, sizeof(adress)) == -1)
  {
    fprintf(stderr,"ERROR: couldn't estabilish connection between socket and interface!\n");
    return ERR_BIND;
  }

/* Prosmicuous Mode */
  memset(&mr, 0, sizeof(mr));          
  mr.mr_ifindex = ir.ifr_ifindex;
  mr.mr_type = PACKET_MR_PROMISC;
  
  if (setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
  {
    fprintf(stderr, "ERROR: couldn't activate prosmicuous mode!\n");
    return ERR_ACTIVATION;
  }

  return sock;
}


/* Creates or modifies a packet with the given parameters 
   RETURN:
    - A pointer to the packet if the packet was created or modified
*/
packet_t *create_or_modify_packet(packet_t *packet, uint8_t size, uint8_t sequence, uint8_t type, void *data) 
{   
    if (packet == NULL) 
    {
        packet = calloc(1, sizeof(packet_t));
        if (packet == NULL) 
        {
            fprintf(stderr, "ERROR: packet allocation failure!");
            exit(EXIT_FAILURE);
        }
    }
    else if (!packet_verification(size, sequence, type)) 
    {
        fprintf(stderr, "ERROR: invalid parameters for packet!");
        destroy_packet(packet);
        return NULL;
    }

    packet->start_marker = START_MARKER;
    packet->size = size;    
    packet->sequence = sequence;
    packet->type = type;

    memset(packet->data,0, DATA_SIZE);

    if (size > 0)
    {
        uint8_t *data_bytes = (uint8_t *)data; 
        if(data_bytes[DATA_SIZE-1] == 0x01)
            memcpy(&packet->data, data, size+1);
        else
            memcpy(&packet->data, data, size);
    }

    packet->crc8 = crc8_calc(packet);

    return packet;
}

/* Send a packet, if exists 
   RETURN:
    - 0 if the packet was sent with success.
    - -1 if an error occurred.
*/
int send_packet(packet_t *packet, int socket)
{

    shift_bits(packet);

    if(send(socket, packet, sizeof(packet_t), 0) == -1) 
    {
        fprintf(stderr, "ERROR: couldn't send packet!\n");
        close(socket);
        exit(EXIT_FAILURE);
    }

    unshift_bits(packet);

    return 0;
}

/* 
   Sends a packet and waits for a response, ACK or ERROR. 
   If the response is a NACK, the packet is sent again.
   Return:
     0 if the packet was sent and the response was received.
    -1 if an error occurred.
    -2 if the timeout expired.
*/
int send_packet_stop_wait(packet_t *packet, packet_t *response, int timeout, int socket)
{
    int try = 0;

    packet_t *aux_packet = create_or_modify_packet(NULL, 0, 0, ERROR, NULL);

    while(1)
    {
        send_packet(packet, socket);
        
        int listen_response = listen_for_packet(aux_packet, timeout, socket);
        if(listen_response == ERR_TIMEOUT_EXPIRED) // Timeout, try until MAX_TRY
        {
            try++;
            if(try > MAX_TRY)
                return ERR_TIMEOUT_EXPIRED;
            printf("Waiting server...");
            continue;
        }
            
        if(listen_response == ERR_LISTEN) // Error
            return ERR_LISTEN;

        else if(aux_packet->type == ACK || aux_packet->type == ERROR || aux_packet->type == NACK)
            break;
    }
    
    #ifdef DEBUG
    response_reply(aux_packet); // Just for tests
    #endif
    
    create_or_modify_packet(response, aux_packet->size, aux_packet->sequence, aux_packet->type, aux_packet->data);
    destroy_packet(aux_packet);

    return 0;
}

/* Listens for a valid packet
    Type of return:
    0 if the packet was received with success.
   -1 if an error occurred. 
   -2 if the timeout expired. 
*/
int listen_for_packet(packet_t *buffer, int timeout, int socket)
{
    fd_set rfds;
    struct timeval t_out;

    /* Set the socket to listen */
    FD_ZERO(&rfds);
    FD_SET(socket, &rfds);

    /* Timeout and start, now time */
    t_out.tv_usec = 0;
    clock_t start = clock();
    clock_t now = clock();

    /* While the timeout is not expired */
    while(diff_time(start, now) < timeout)
    {
        memset(buffer, 0, sizeof(packet_t)); // Reset the buffer

        t_out.tv_sec = timeout - diff_time(start, now); // If the timeout is not expired, update the time

        int ready = select(socket + 1, &rfds, NULL, NULL, &t_out);
        
        if (ready == ERR_LISTEN) 
            return ERR_LISTEN; // Error 
        else if (ready == 0) 
        {
            return ERR_TIMEOUT_EXPIRED; // Timeout expired
        } 
        else 
        {

            ssize_t bytes_received = recv(socket, buffer, sizeof(packet_t), 0);
            unshift_bits(buffer);

            if(bytes_received == ERR_LISTEN) 
                return ERR_LISTEN;
            
            /* Checks if the packet is from client and if it's crc8 is right  */ 
            if(start_marker_verification(buffer) )
            {
                if (crc8_verification(buffer) == 0) // Verify CRC8
                {   
                    packet_t *nack = create_or_modify_packet(NULL, 0, buffer->sequence, NACK, NULL);
                    send_packet(nack, socket);
                    destroy_packet(nack);
                }
                else // The pacet is valid
                    return VALID_PACKET;
            }
        }
        now = clock(); // Update the time
    }
    return ERR_TIMEOUT_EXPIRED; 
}

/* Destory a packet, if exists */
void destroy_packet(packet_t *p) 
{
    if (p == NULL) 
        return;
    free(p);
}

/* Print the type of a packet, auxilary function */
void response_reply(packet_t *p)
{
    switch (p->type)
    {
    case ACK:
        print_log("ACK recebido!");
        break;
    case NACK:
        print_log("NACK recebido!");
        break;
    case ERROR:
        print_log("ERROR recebido!");
        break;
    }
}

/* *** Auxiliary Functions *** */

/* Calculate the CRC8 */
uint8_t crc8_calc(packet_t *packet) 
{
    uint8_t data[3 + DATA_SIZE];

    size_t length;  
    length = DATA_SIZE + 3;
    data[0] = packet->size;
    data[1] = packet->sequence;
    data[2] = packet->type;

    memset(data+3, 0, DATA_SIZE);

    if(packet->size > 0)   
        memcpy(data+3, packet->data, packet->size);

    uint8_t crc = 0x00;
    const uint8_t generator = 0x07; // Polynomial   
    for (size_t j = 0; j < length; j++) {
      crc ^= data[j];   
      // For each bit of the byte check for the MSB bit, if it is 1 then left
      // shift the CRC and XOR with the polynomial otherwise just left shift the
      // variable
      for (int i = 0; i < 8; i++) {
        if ((crc & 0x80) != 0) {
          crc = (crc << 1)^ generator;
        } else {
          crc <<= 1;
        }
      }
    }   
    return crc;
}


/* Return the time passed between two clock_t */
double diff_time(clock_t start, clock_t end)
{
    return ((double)(end - start) / CLOCKS_PER_SEC);
}

/* Checks if the start marker of a packet is correct */
int start_marker_verification(packet_t *p)
{
    if(p->start_marker != (uint8_t)(START_MARKER))
        return 0;

    return 1;
}

/* Verify packet parameters */
int packet_verification(uint8_t size, uint8_t sequence, uint8_t type) 
{
    if ((size > MAX_DATA_SIZE) || (sequence > MAX_SEQUENCE) || (type > MAX_TYPE)) 
        return 0;

    return 1;
}

/* Checks if the crc8 of a packet is correct */
int crc8_verification(packet_t *p)
{
    if(p->crc8 != crc8_calc(p))
        return 0;

    return 1;
}

/* Shift bits function */
void shift_bits(packet_t* p) {
    p->size = p->size << 2; // shift left 2 bits to use upper 6 bits
    p->sequence = p->sequence << 3; // shift left 3 bits to use upper 5 bits
    p->type = p->type << 3; // shift left 3 bits to use upper 5 bits
}

/* Unshift bits function */
void unshift_bits(packet_t* p) {
    p->size = p->size >> 2; // shift right 2 bits to get back original 6 bits
    p->sequence = p->sequence >> 3; // shift right 3 bits to get back original 5 bits
    p->type = p->type >> 3; // shift right 3 bits to get back original 5 bits
}


