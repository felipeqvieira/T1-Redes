#ifndef COMMAND_H
#define COMMAND_H

#include "../lib/connection.h"

/* List the video files in the directory */
int list_video_files_in_directory(char *directory, int socket);

/* Send a video file with sliding window */
int send_video(char *file_name, int socket);

/* Make the download of the select video */
int download_video(char *file_name, int socket);

/* Receive a video file */
int receive_video(char *file_path,  int socket, size_t file_size);

#endif
