#include "../lib/utils.h"
#include "../lib/command.h"

#define DT_REG 8

/* Verify if the file have a video extension */
int is_video_file(const char *filename);

/* Verify if the file have a video extension */
int is_video_file(const char *filename)
{
    const char *video_extensions[] = { ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", NULL };
    const char **ext = video_extensions;

    while (*ext)
    {
        if (strstr(filename, *ext))
        {
            return 1;
        }
        ext++;
    }
    return 0;
}

/* List the videos in a directory and return for the client */
int list_video_files_in_directory(char *directory, int socket)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(directory);
    if (d == NULL) 
    {
        fprintf(stderr,"opendir");
        return -1;
    }

    char file_list[DATA_SIZE];
    memset(file_list, 0, DATA_SIZE);
    packet_t response_packet;
    packet_t *packet= create_or_modify_packet(NULL, 0, 0, ACK, NULL);

    while ((dir = readdir(d)) != NULL) 
    {
        if (dir->d_type == DT_REG && is_video_file(dir->d_name))
        {
            size_t name_len = strlen(dir->d_name); 
            if (name_len <= MAX_FILE_NAME_SIZE)  
            {
                memcpy(file_list, dir->d_name, name_len);
                create_or_modify_packet(packet, name_len, 0, SHOW_IN_SCREEN, file_list);
                send_packet_stop_wait(packet, &response_packet, TIMEOUT, socket);
                if(response_packet.type == ERROR)
                {
                    print_log("Error while sending file list to client!");
                    destroy_packet(packet);
                    return -1;
                }
                 memset(file_list, 0, MAX_FILE_NAME_SIZE);
            }
        }
    }
    closedir(d);

    create_or_modify_packet(packet, 0, 0, END_TRANSMISSION, NULL);
    send_packet_stop_wait(packet, &response_packet, TIMEOUT, socket);
    destroy_packet(packet);
    return 0;
}

/* Send a video file with sliding window */
int send_video(char *file_name, int socket)
{
    if(file_name == NULL)
    {
        fprintf(stderr, "Path is NULL!");
        return ERR_PATH;
    }

    FILE *file = fopen(file_name, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "Could not open file!");
        return ERR_FILE;
    }


    uint8_t data_buffer[DATA_SIZE] = {0};
    size_t file_size = get_file_size(file_name);
    memcpy(data_buffer, &file_size, sizeof(file_size));
    
    struct tm *time_info = get_file_date(file_name);
    snprintf((char*)(data_buffer+43), 20, "%04u-%02u-%02u %02u:%02u:%02u", 
            time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
            time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

    struct packet *p = create_or_modify_packet(NULL, MAX_DATA_SIZE, 0, DESCRIPTOR, data_buffer );

    if (send_packet_stop_wait(p, p, TIMEOUT, socket) != 0)
    {
        print_log("Error while trying to reach server!\n");
        return -1;
    }

    if(p->type == ERROR)
    {
        char *error_msg = convert_to_string(p->data, p->size);
        fprintf(stderr, "ERROR: %s\n", error_msg);
        create_or_modify_packet(p, 0, 0, ACK, NULL);
        send_packet(p, socket);
        destroy_packet(p);
        free(error_msg);
        fclose(file);
        return ERROR;
    }

    struct packet p_buffer;
    bool inRange;
    size_t file_read_bytes;
    long long packets_quantity = ceil((double) file_size / (double)(MAX_DATA_SIZE));
    long long int next_seq = 0, base = 0;
    int listen, try = 0;

    struct packet **window = (struct packet **) malloc(WINDOW_SIZE * sizeof(struct packet *));
    memset(window,0, WINDOW_SIZE * sizeof(struct packet*));

    int window_start = base % WINDOW_SIZE;
    int window_end = (base + WINDOW_SIZE - 1) % WINDOW_SIZE;

    while(1)
    {
        while(next_seq < base + WINDOW_SIZE && next_seq < packets_quantity)
        {
            file_read_bytes = fread(data_buffer, 1, MAX_DATA_SIZE, file);
            replace_bytes_server(data_buffer, DATA_SIZE, 0x88, 0xA8, 0xFF, 0xFF);
            replace_bytes_server(data_buffer, DATA_SIZE, 0x81, 0x00, 0xEE, 0xEE);
            long long int seq = next_seq % (MAX_SEQUENCE + 1);
            int index = next_seq % WINDOW_SIZE;
            window[index] = create_or_modify_packet(NULL, file_read_bytes, seq , DATA, data_buffer);
            send_packet(window[index], socket);
            next_seq++;
            memset(data_buffer, 0, DATA_SIZE);
        }

        listen = listen_for_packet(p, TIMEOUT, socket) ;

        if(listen != 0)
        {
            try++;
            if(try > MAX_TRY)
                return ERR_TIMEOUT_EXPIRED;
        }
        if(listen == 0)
        {
            try = 0;
            if(p->type == ACK)
            { 
                if(window_start <= window_end)  
                    inRange = ((p->sequence % WINDOW_SIZE) >= window_start) && ((p->sequence % WINDOW_SIZE) <= (window_end));
                else
                    inRange = ((p->sequence % WINDOW_SIZE) >= window_start) || ((p->sequence % WINDOW_SIZE) <= (window_end));

                if(inRange)
                {
                    while((base % (MAX_SEQUENCE + 1)) != p->sequence)
                    {
                        free(window[base % WINDOW_SIZE]);
                        base++;
                    }
                    free(window[base % WINDOW_SIZE]);
                    base++;
                    window_start = base % WINDOW_SIZE;
                    window_end = (base + WINDOW_SIZE - 1) % WINDOW_SIZE;
                }   
            }
            else if(p->type == NACK)
            {
                printf("Resend window\n");
                for(int i = 0; i < WINDOW_SIZE; i++)
                    send_packet(window[i], socket);   
            }
        }      
        else if(listen == ERR_TIMEOUT_EXPIRED)
        {
            printf("Resend window\n");
            for(int i = 0; i < WINDOW_SIZE; i++)
                send_packet(window[i], socket);
        }

        printf("\r%s: ", file_name);
        fflush(stdout);
        print_progress(file_size, next_seq, sizeof(data_buffer));
        
        if(base == packets_quantity)
        {
            free(window);
            break;
        }

    }
    printf("\n");
    fclose(file);

    /* Send end transmission packet. */
    create_or_modify_packet(p, 0, 0, END_TRANSMISSION, NULL);
    if (send_packet_stop_wait(p, &p_buffer, TIMEOUT, socket) != 0)
    {
        destroy_packet(p);
        return -1;
    }

    print_log("File sent successfully!");

    destroy_packet(p);

    return 0;
}

/* Receive a video */
int receive_video(char *file_name, int socket, size_t file_size)
{
    FILE *file = fopen(file_name, "wb");
    if (file == NULL)
    {
        fprintf(stderr,"Error opening the file");
        return -1;
    }

    packet_t *packet_buffer = create_or_modify_packet(NULL,0,0,ACK,NULL);
    packet_t *response = create_or_modify_packet(NULL, 0, 0, ACK, NULL);
    long long int packets_received = 0;
    int expected_seq = 0, seq = 0, listen, try = 0;
    
    while (1)  
    {   
        
        /* Show download progress bar */
        printf("\r%s: ", file_name);
        print_progress(file_size, packets_received, sizeof(packet_buffer->data));
        fflush(stdout);

        /* Listen for packets */
        memset(packet_buffer, 0, sizeof(packet_t));
        listen = listen_for_packet(packet_buffer, TIMEOUT, socket);

        if (listen != 0) 
        {
            try++;
            if(try > MAX_TRY) // Try until MAX_TRY
                return ERR_TIMEOUT_EXPIRED;
            printf("Waiting server...\n");
            continue;
        }

        if (packet_buffer->type == END_TRANSMISSION) // End of transmission
        {
            break;
        }
        else if (packet_buffer->type == DATA) // Packets
        {
            try = 0;
            replace_bytes_client(packet_buffer->data, DATA_SIZE, 0xFF, 0xFF, 0x88, 0xA8); 
            replace_bytes_client(packet_buffer->data, DATA_SIZE, 0xEE, 0xEE, 0x81, 0x00);
            seq = packet_buffer->sequence;
            expected_seq = packets_received % (MAX_SEQUENCE + 1);
            if(seq == expected_seq) // If the packet is the expected one
            {   
                fwrite(packet_buffer->data, 1, packet_buffer->size, file);
                create_or_modify_packet(response, 0, expected_seq, ACK, NULL);
                send_packet(response, socket);
                packets_received++;
            }
        }
    }

    printf("\n");
    fclose(file);

    /* Send ACK for end transmision packet */
    create_or_modify_packet(response, 0, 0, ACK, NULL);
    send_packet(response, socket);

    printf("%s downloaded!\n", file_name);

    destroy_packet(response);
    destroy_packet(packet_buffer);

    return 0;
}

/* Make the download of the select video */
int download_video(char *file_name, int socket)
{
    packet_t *p = create_or_modify_packet(NULL, MAX_FILE_NAME_SIZE, 0, DOWNLOAD, file_name);

    int response = send_packet_stop_wait(p, p, TIMEOUT, socket);

    /* Send packet for start to download file */
    if(response == ERR_LISTEN)
    {
        fprintf(stderr, "ERROR: couldn't send packet to download file!");
        destroy_packet(p);
        return ERR_LISTEN;
    }
    if(response == ERR_TIMEOUT_EXPIRED)
    {
        fprintf(stderr, "ERROR: couldn't send packet to download file!");
        destroy_packet(p);
        return ERR_TIMEOUT_EXPIRED;
    }

    if(p->type == ERROR)
    {
        char *error_msg = convert_to_string(p->data, p->size);
        fprintf(stderr, "ERROR: %s\n", error_msg);
        create_or_modify_packet(p, 0, 0, ACK, NULL);
        send_packet(p, socket);
        destroy_packet(p);
        free(error_msg);
        return ERROR;
    }

    /* Listen for descriptor packet */
    if (listen_for_packet(p, TIMEOUT, socket) != 0)
    {
        fprintf(stderr, "ERROR: couldn't listen for descriptor packet!");
        destroy_packet(p);
        return ERR_LISTEN;
    }

    /* Extract information from DESCRIPTOR packet */
    size_t extracted_size = 0;
    for (size_t i = 0; i < 4; ++i)
        extracted_size |= ((size_t)p->data[i]) << (i * 8);

    char data_str[20];
    memcpy(data_str, p->data + 43, 20);
    data_str[21] = '\0';

    /* Verify if it's the same file */
    if(access(file_name, F_OK) == 0)
    {
        char file_date_str[21]; 
        file_date_str[21] = '\0';
        struct tm *time_info = get_file_date(file_name);
        snprintf((char*)(file_date_str), 20, "%04u-%02u-%02u %02u:%02u:%02u", 
            time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,
            time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
        if(strcmp(data_str, file_date_str) == 0)
        {
            create_or_modify_packet(p, MAX_DATA_SIZE, 0, ERROR, "File already exists!");
            send_packet_stop_wait(p, p, TIMEOUT, socket);
            printf("File %s already exists!\n", file_name);
            destroy_packet(p);
            return 0;
        }
    }


    /* Verify if the file can be downloaded */
    if(can_download_file(extracted_size) == 0)
    {
        create_or_modify_packet(p, MAX_DATA_SIZE,0, ERROR, "DISK FULL!");

        send_packet_stop_wait(p, p, TIMEOUT, socket);
        destroy_packet(p);
        printf("The file couldn't be downloaded, size is greater than the free space in disk!\n");
        return ERR_DISK_FULL;
    }

    create_or_modify_packet(p, 0, 0, ACK, NULL);
    send_packet(p, socket);

    if(receive_video(file_name, socket, extracted_size) != 0)
    {
        fprintf(stderr,"ERROR: couldn't download the video, please try again!\n");
        destroy_packet(p);
        return ERR_RECEIVE;
    }


    /* Setting the file date */
    struct tm *file_date = get_file_date(file_name);
    convert_to_tm_struct(data_str, file_date);
    set_file_date(file_name, file_date);

    destroy_packet(p);

    return 0;
}
