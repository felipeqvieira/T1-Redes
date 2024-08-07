#include "../lib/utils.h"

/* Get the size of a file in bytes */ 
long long int get_file_size(char *file_name) 
{
    struct stat file_stat;
    if(stat(file_name, &file_stat) == -1) 
    {
        fprintf(stderr, "ERROR: couldn't get file size!\n");
        return -1;
    }
    return file_stat.st_size;
}

/* Get the date of a file */
struct tm *get_file_date(char *file_name)
{
    struct stat file_stat;
    if(stat(file_name, &file_stat) == -1)
    {
        fprintf(stderr, "ERROR: couldn't get file date!\n");
        return NULL;
    }
    return localtime(&file_stat.st_mtime);
}

/* Set the date of a file */
int set_file_date(char *file_name, struct tm *new_date)
{
    struct utimbuf new_times;
    new_times.actime = mktime(new_date);  // Set access time to the new date
    new_times.modtime = mktime(new_date); // Set modification time to the new date
    
    if (utime(file_name, &new_times) == -1)
    {
        fprintf(stderr, "ERROR: couldn't set file date!\n");
        return -1;
    }
    return 0;
}

/* Get the last modification time of a file */
time_t get_file_modification_time(char *path) 
{
    struct stat st;
    if(stat(path, &st) == -1) 
    {
        fprintf(stderr,"Could not get file modification time!");
        return -1;
    }
    return st.st_mtime;
    
}

/* Do the conversion of an array of type uint8 to an array of type String
    RETURN:
     - A pointer to the string
     - NULL if the memory allocation failed
*/
char* convert_to_string(uint8_t* array, size_t length)
{
    char* str = (char*)malloc((length + 1) * sizeof(char));  // Allocate memory for the string
    
    if (str == NULL) {
        fprintf(stderr, "ERROR: memory allocation failed!\n");
        return NULL;
    }
    
    for (size_t i = 0; i < length; i++) {
        str[i] = (char)array[i];  // Convert each element to char and store in the string
    }
    
    str[length] = '\0';  // Add the null-terminating character
    
    return str;
}

/* Print an art for the program */
void printFlix()
{
    printf("\t$$$$$$$$\\ $$\\      $$$$$$\\ $$\\   $$\\       \n");
    printf("\t$$  _____|$$ |     \\_$$  _|$$ |  $$ |      \n");
    printf("\t$$ |      $$ |       $$ |  \\$$\\ $$  |      \n");
    printf("\t$$$$$\\    $$ |       $$ |   \\$$$$  /       \n");
    printf("\t$$  __|   $$ |       $$ |   $$  $$<        \n");
    printf("\t$$ |      $$ |       $$ |  $$  /\\$$\\       \n");
    printf("\t$$ |      $$$$$$$$\\$$$$$$\\ $$ /  $$ |      \n");
    printf("\t\\__|      \\________\\______|\\__|  \\__|      \n");
}


/* Get the current directory */
void get_directory(char *buffer, size_t size)
{
    if (getcwd(buffer, size) == NULL) {
        fprintf(stderr, "ERROR: couldn't get the current directory!\n");
    }
    // add "/" to the end of the path
    strcat(buffer, "/");
}


/* Show the packet info */
int show_packet_data(packet_t *p){

    if(p == NULL){
        fprintf(stderr, "ERROR: couldn't print packet data!\n");
        return -1;
    }

    printf("Packet Type: %X\n", p->type);
    printf("Packet Size: %d\n", p->size);
    printf("Packet Data: %s\n", p->data);
    printf("Packet Sequence Number: %d\n", p->sequence);
    printf("Packet crc8: %d\n", p->crc8);
    
    return 0;
}

/* Print a log message with a timestamp */
void print_log(const char* msg)
{
   time_t now;
   time(&now);

   struct tm* local_time = localtime(&now);
   
   char time_stamp[20];
   strftime(time_stamp, sizeof(time_stamp), "%d-%m-%Y %H:%M:%S", local_time);

   printf("%s - %s\n", msg , time_stamp);
}

/* Print the progress of a download */
void print_progress(size_t total_size, size_t received_packets, size_t packet_size) {
    if (total_size == 0 || packet_size == 0) {
        fprintf(stderr, "ERROR: total size or packet size is invalid.\n");
        return;
    }

    size_t received_size = received_packets * packet_size;
    double progress = (double)received_size / total_size * 100.0;

    // Limiting of 0 to 100%
    if(progress > 100.0)
        progress = 100.0;

    int bar_width = 50; // progress bar width
    int pos = (int)(bar_width * (progress / 100.0));

    // Show the progress bar
    printf("[");
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos)
            printf("=");
        else if (i == pos)
            printf(">");
        else
            printf(" ");
    }
    printf("] %.2f%%\r", progress);

    // Clear the console output
    fflush(stdout);
}

/* Get the free space in disk  */
unsigned long long get_free_space(const char *path) {
    struct statfs stat;

    // Obtains information about the filesystem
    if (statfs(path, &stat) != 0) {
        fprintf(stderr, "ERROR: couldn't get filesystem information\n");
        return 0;
    }

    // Returns the free space in bytes
    return (unsigned long long) stat.f_bsize * stat.f_bavail;
}

/* Verify if there is enough space in disk to download a file 
   RETURN:
    1 - Enough space
    0 - Not enough space
*/
int can_download_file(size_t video_size) {
    unsigned long long free_space = get_free_space("/");

    if(video_size > free_space) 
    {
        #ifdef DEBUG
            printf("Not enough space in disk to download file\n");
        #endif
        return 0;
    } else 
    {
        #ifdef DEBUG
            printf("Enough space in disk to download file\n");
        #endif
        return 1;
    }
}

void convert_to_tm_struct(char *date_str, struct tm *tm)
{
    memset(tm, 0, sizeof(struct tm));
    sscanf(date_str, "%4d-%2d-%2d %2d:%2d:%2d",
           &tm->tm_year, &tm->tm_mon, &tm->tm_mday,
           &tm->tm_hour, &tm->tm_min, &tm->tm_sec);
    tm->tm_year -= 1900;
    if(tm->tm_mon > 0)
        tm->tm_mon -= 1; // tm_mon vai de 0 a 11
    else
        tm->tm_mon = 0;
}


void replace_bytes_client(uint8_t *buffer, size_t size, uint8_t byte1, uint8_t byte2, uint8_t new_byte1, uint8_t new_byte2) {
    
    if(buffer[size-1] != 0x01)
        return;

    for (size_t i = 0; i < size - 1; ++i)
    {
        if (buffer[i] == byte1 && buffer[i+1] == byte2) {
            if(i == 8)
            {
                buffer[i] = new_byte1;
                buffer[i+1] = new_byte2;
                buffer[size-1] = 0x00;

            }
        }
    }
}

void replace_bytes_server(uint8_t *buffer, size_t size, uint8_t byte1, uint8_t byte2, uint8_t new_byte1, uint8_t new_byte2) {
    for (size_t i = 0; i < size - 1; ++i) {
        if (buffer[i] == byte1 && buffer[i+1] == byte2) {
            if(i == 8)
            {
                buffer[i] = new_byte1;
                buffer[i+1] = new_byte2;
                buffer[size-1] = 0x01;
                return;
            }
        }
    }
}