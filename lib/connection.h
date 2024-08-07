#ifndef CONNECTION_H 
#define CONNECTION_H

#include <stdint.h> // uint8_t
#include <stdio.h> // Input and Output
#include <stdlib.h> // Memory allocation
#include <string.h> // String manipulation
#include <math.h> // Math functions
#include <dirent.h> // Directory manipulation
#include <time.h> // Time functions
#include <stdbool.h> // Boolean values
#include <unistd.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h> // 
#include <linux/if.h> // 
#include <netinet/ip.h> // Interface
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <utime.h>

/* Always include this value in the start of the packet */
#define START_MARKER 0x7E

/* Protocol type codes */
#define ACK 0x00
#define NACK 0x01
#define LIST 0x0A
#define DOWNLOAD 0X0B
#define SHOW_IN_SCREEN 0x10
#define DESCRIPTOR 0x0D
#define DATA 0x0C
#define END_TRANSMISSION 0x9
#define ERROR 0x1F
#define ONLINE 0x11

/* Just for tests */
#define SERVER_OFF 0x12

/* Maximum values for the protocol */
#define MAX_FILE_NAME_SIZE 63 
#define DATA_SIZE 64 // 0 to 63
#define MAX_DATA_SIZE 63 // 6 bits for size
#define MAX_SEQUENCE 31 // 5 bits for sequence
#define MAX_TRY 16
#define MAX_TYPE 31 // 5 bits for type

/* Window size and timeout */
#define WINDOW_SIZE 5
#define TIMEOUT 5 // In seconds

/* Return code for Errors */
#define ACESS_DENIED -1
#define FILE_NOT_FOUND -2
#define INVALID_PARAMETER -44
#define EXEC_ABORTATION -1

/* Return code for network errors */
#define ERR_PERMISSION -1
#define ERR_INTERFACE -2
#define ERR_BIND -3
#define CRC_ERROR -4
#define ERR_TIMEOUT -5
#define ERR_ACTIVATION -4
#define ERR_NACK -3

#define ERR_LISTEN -1
#define ERR_TIMEOUT_EXPIRED -2
#define ERR_DISK_FULL -3
#define ERR_RECEIVE -4

#define ERR_PATH -1
#define ERR_FILE -2

/* Anothe constants */
#define VALID_PACKET 0



/* Struct to represent the protocol packet based on Kermit */
typedef struct packet {
    uint8_t start_marker; // 1 byte
    uint8_t size; // 6 bits
    uint8_t sequence; // 5 bits 
    uint8_t type; // 5 bits
    uint8_t data[DATA_SIZE]; // Max length of data is 64 bytes
    uint8_t crc8; // 8 bits (size, sequence, type, data)
} packet_t;


/* Create and bind a socket to the selected device */
int create_socket(char *device);

/* Send a packet */
int send_packet(packet_t *p, int socket);

/* Send a packet and wait for receive - Stop and Wait */
int send_packet_stop_wait(packet_t *packet, packet_t *response, int timeout, int socket);

/* Verifiy if the packet is valid */
int is_a_valid_packet(packet_t *p);

/* Listen for a packet */
int listen_for_packet(packet_t *buffer, int timeout, int socket);

/* Free the memory for the packet */
void destroy_packet(packet_t *p);

/*  Create a packet if not exist or modify the parameters for a packet */
packet_t *create_or_modify_packet(packet_t *packet, uint8_t size, uint8_t sequence, uint8_t type, void *data); 

/* Print based of the type of the reply, ACK, NACK or ERROR */
void response_reply(packet_t *p);

#endif