#include "../lib/connection.h"
#include "../lib/command.h"
#include "../lib/utils.h"

// Auxiliary functions
int process_command(char *token, const char delimiter[], int type_flag, int sockfd); // To process what command will execute
void play_video(const char *file_name); // To play the downloaded video
void print_commands(); // To print the available commands
void remove_video(const char *file_name); // To remove the downloaded video after play


int main()
{
  	int sockfd = create_socket("enp1s0f1"); // interface
    char *token;
    char input[100]; // buffer for commands
    char *file_name = NULL;
    char *error = NULL;
    const char delimiter[] = " \n";
    packet_t buffer; // packet buffer
    
    system("clear");
    printf("\n\n");
    printFlix();

    printf("\nEstabilishing connection...\n");
    // Verify if the server is online
    packet_t *packet = create_or_modify_packet(NULL, 0, 0, ONLINE, NULL);
    if((send_packet_stop_wait(packet, packet, TIMEOUT, sockfd)) != 0)
    {
        printf("Server is offline, please try again later.\n");
        destroy_packet(packet);
        return 0;
    }

    system("clear");
    printf("\n\n");
    printFlix();
    printf("\n\n");
    print_commands();
    printf("\n");


    while (1) 
    {
        printf("\nFLIX => ");
        fgets(input, sizeof(input), stdin);

        token = strtok(input, delimiter);

        if(token == NULL) continue;

        if(strcmp(token, "list") == 0)
        {    
            if((process_command(token, delimiter, LIST, sockfd)) != 0)
                continue; // In case of error continue
            buffer.type = LIST; 
        }    
        else if(strcmp(token, "download") == 0)
        {
            if((process_command(token, delimiter, DOWNLOAD,sockfd)) != 0 )
                continue; // In case of error continue
            buffer.type = END_TRANSMISSION; // To no enter in the while loop
        }
        else if(strcmp(token, "help") == 0)
        {
            print_commands();
            continue;
        }
        else if(strcmp(token, "exit") == 0)
        {
            destroy_packet(packet);
            printf("Bye, hope you enjoy it!\n");
            sleep(1);
            system("clear");
            return 0;
        }
        else if(strcmp(token, "off") == 0) // Just for testing leakage
        {
            create_or_modify_packet(packet, 0, 0, SERVER_OFF, NULL);
            send_packet_stop_wait(packet, packet, TIMEOUT, sockfd);
            destroy_packet(packet);
            return 0;
        }
        else if(strcmp(token, "clear") == 0)
        {
            system("clear");
            continue;
        }
        else
        {
            printf("Command doesn't exist: %s\n", token);
            print_commands();
            continue;
        }

        // Loop for list the packets
        while(buffer.type != END_TRANSMISSION)
        {

           listen_for_packet(&buffer, 9999, sockfd);
            switch(buffer.type)  
            {
            case ACK:
                #ifdef DEBUG
                print_log("ACK received!");
                #endif
                break;

            case NACK:
                #ifdef DEBUG
                print_log("NACK received!");
                #endif
                break;

            case SHOW_IN_SCREEN:
                #ifdef DEBUG
                print_log("SHOW_IN_SCREEN received!");
                #endif
                file_name = convert_to_string(buffer.data, buffer.size);
                printf("%s\n", file_name);
                create_or_modify_packet(packet, 0, 0, ACK, NULL);
                send_packet(packet, sockfd);
                free(file_name);
                break;

            case ERROR:
                #ifdef DEBUG
                print_log("ERROR received!");
                #endif
                error = convert_to_string(buffer.data, buffer.size);
                printf("%s\n", error);
                create_or_modify_packet(packet, 0, 0, ACK, NULL);
                send_packet(packet, sockfd);
                break;

            case END_TRANSMISSION:
                #ifdef DEBUG
                print_log("END_OF_TRANSMISSION received!");
                #endif
                create_or_modify_packet(packet, 0, 0, ACK, NULL);
                send_packet(packet, sockfd);
                break;
            }
        }
        
        // Reset the buffer
        buffer.start_marker = 0;
        buffer.type = 0;
    }

    close(sockfd);
    return 0;
}


int process_command(char *token, const char delimiter[], int type_flag, int sockfd)
{

    packet_t *packet = create_or_modify_packet(NULL, 0, 0, ACK, NULL);


    if(type_flag == LIST)
    {
        create_or_modify_packet(packet, 0, 0, LIST, NULL);
        if((send_packet_stop_wait(packet, packet, TIMEOUT, sockfd)) != 0)
        {
            printf("Error while sending list packet!\n");
            destroy_packet(packet);
            return -1;
        }
        destroy_packet(packet);
        printf("Available videos to watch:\n");
    }
    else if(type_flag == DOWNLOAD)
    {
        token = strtok(NULL, delimiter);
        if(token == NULL)
        {
            printf("Please, type the command again with the name of a video to download.\n");
            return -1;
        }
        char video_name[DATA_SIZE];
        memset(video_name, 0, DATA_SIZE);
        strncpy(video_name, token, DATA_SIZE);
        printf("Downloading: %s\n", token);
        if((download_video(video_name, sockfd)) != 0)
            return -1;
        destroy_packet(packet);
        printf("--> Playing video\n");
        // system("clear");
        play_video(video_name);
        // remove_video(token);
        
    }
    return 0;
}

void print_commands()
{
    printf("Available commands:\n");
    printf("- list : Show a list of the available videos.\n");
    printf("- download <name> : Download and play the selected video.\n");
    printf("- help: Show this message\n");
    printf("- exit: Exit the program.\n");
}

void play_video(const char *file_name)
{
    char command[256];
    
    snprintf(command, sizeof(command), "mplayer %s > /dev/null 2>&1", file_name);
    
    int result = system(command);
    
    if (result == -1) {
        fprintf(stderr,"Error trying to execute the video");
    } else {
        return;
    }
}

void remove_video(const char *file_name)
{
    char command[256];
    
    snprintf(command, sizeof(command), "rm -f %s", file_name);
    
    int result = system(command);
    
    if (result == -1) {
        fprintf(stderr, "Error trying to remove the video");
    } else {
        return;
    }
}