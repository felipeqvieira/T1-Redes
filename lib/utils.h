#ifndef UTILS_H
#define UTILS_H

#include "../lib/connection.h"

/* Convert a uint8 array to string */
char* convert_to_string(uint8_t* array, size_t length);

/* Print the logo of Flix */
void printFlix();

/* Get the current directory */
void get_directory(char *buffer, size_t size); 

/* Get the file size */
long long int get_file_size(char *file_name);

/* Print a log message, with the time */
void print_log(const char* msg);

/* Show the packet information */
int show_packet_data(packet_t *p);

/* Print the progress of the download in a bar */
void print_progress(size_t total_size, size_t received_packets, size_t packet_size);

/* Get the date of a file */
struct tm *get_file_date(char *file_name);

/* Set the date of a file */
int set_file_date(char *file_name, struct tm *new_date);

/* Convert a string to a tm struct */
void convert_to_tm_struct(char *date_str, struct tm *tm);

/* Verifiy if the file can be downloaded */
int can_download_file(size_t video_size);

/* Convert a string to time_t */
time_t convert_to_time_t(char* date_str);

/* Replace the bytes in the buffer, for client */
void replace_bytes_client(uint8_t *buffer, size_t size, uint8_t byte1, uint8_t byte2, uint8_t new_byte1, uint8_t new_byte2);

/* Replace the bytes in the buffer, for server */
void replace_bytes_server(uint8_t *buffer, size_t size, uint8_t byte1, uint8_t byte2, uint8_t new_byte1, uint8_t new_byte2);


#endif  // UTILS_H