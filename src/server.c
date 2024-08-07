#include "../lib/command.h"
#include "../lib/utils.h"
#include "../lib/connection.h"

int main()
{
    system("clear");
    print_log("Creating socket...");
    int socket = create_socket("eno1");
    print_log("Socket created!");
    print_log("Server online and working");
    char current_directory[100];
    get_directory(current_directory, sizeof(current_directory));
    printf("Server location %s\n", current_directory);

    packet_t buffer;
    packet_t *packet = create_or_modify_packet(NULL, 0, 0, ACK, NULL);

    char *file_name = NULL;
    while (1)
    {
        print_log("Waiting request from client...");
        listen_for_packet(&buffer, 9999, socket);
        switch(buffer.type)  
        {

        case ONLINE:
            print_log("ONLINE received!");
            create_or_modify_packet(packet, 0, 0, ACK, NULL);
            send_packet(packet, socket);
        break;

        case ACK:
            print_log("ACK received!");
        break;

        case NACK:
            print_log("NACK received!");
        break;

        case LIST:
            print_log("LIST received!");
            create_or_modify_packet(packet, 0, 0, ACK, NULL);
            send_packet(packet, socket);
            list_video_files_in_directory(current_directory, socket);
        break;

        case DOWNLOAD:
            print_log("DOWNLOAD received!");

            file_name = convert_to_string(buffer.data, buffer.size);
            if (access(file_name, F_OK) != 0) // Check if the file exists
            {
                print_log("Video doesn't exists!");
                create_or_modify_packet(packet, MAX_DATA_SIZE, 0, ERROR, "Video doesn't exists");
                send_packet(packet, socket);
                free(file_name);
                break;
            }
            else // The file exists
            {
                print_log("Requested video exists!");
                create_or_modify_packet(packet, 0, 0, ACK, NULL);
                send_packet(packet, socket);
            }

            printf("Sending ==> ");
            printf("%s\n",file_name);
            send_video(file_name, socket);
            free(file_name);
        break;

        case END_TRANSMISSION:
            create_or_modify_packet(packet, 0, 0, ACK, NULL);
            send_packet(packet, socket);
        break;

        case SERVER_OFF: // Just for tests
            print_log("EXIT received!");
            create_or_modify_packet(packet, 0, 0, ACK, NULL);
            send_packet(packet, socket);
            destroy_packet(packet);
            close(socket);
            return 0;
            break;

        default:
            printf("Invalid package received!\n");
            break;
        }
    
    }

    destroy_packet(packet);
    close(socket);
    
    return 0;
}