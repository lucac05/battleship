#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>



#define PORT_1 2201
#define PORT_2 2202
#define BUFFER_SIZE 1024

int main() {
    int listen_fd_1, conn_fd_1 = -1, listen_fd_2, conn_fd_2 = -1;//socket 2
    struct sockaddr_in address_1, address_2;//socket 2
    int opt = 1;
    int addrlen = sizeof(address_1);
    char buffer[BUFFER_SIZE] = {0};
    

    // Create socket
    if ((listen_fd_1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed on port 2201");
        exit(EXIT_FAILURE);
    }

    //socket 2
    if ((listen_fd_2 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed on port 2202");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listen_fd_1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }

    //socket 2
    if (setsockopt(listen_fd_2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }    
    
    // Bind socket to port
    address_1.sin_family = AF_INET;
    address_1.sin_addr.s_addr = INADDR_ANY;
    address_1.sin_port = htons(PORT_1);

    if (bind(listen_fd_1, (struct sockaddr *)&address_1, sizeof(address_1)) < 0) {
        perror("[Server] bind() failed on port 2201");
        exit(EXIT_FAILURE);
    }

    //socket 2
    address_2.sin_family = AF_INET;
    address_2.sin_addr.s_addr = INADDR_ANY;
    address_2.sin_port = htons(PORT_2);

    //socket 2
    if (bind(listen_fd_2, (struct sockaddr *)&address_2, sizeof(address_2)) < 0) {
        perror("[Server] bind() failed on port 2202");
        exit(EXIT_FAILURE);
    }


    // Listen for incoming connections
    if (listen(listen_fd_1, 3) < 0) {
        perror("[Server] listen() failed on port 2201");
        exit(EXIT_FAILURE);
    }

    //socket2
    if (listen(listen_fd_2, 3) < 0) {
        perror("[Server] listen() failed on port 2202");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Running on ports %d and %d\n", PORT_1, PORT_2);

    // Accept incoming connection
    if ((conn_fd_1 = accept(listen_fd_1, (struct sockaddr *)&address_1, (socklen_t *)&addrlen)) < 0) {
        perror("[Server] accept() failed on port 2201");
        exit(EXIT_FAILURE);
    }

    //socket 2
    if ((conn_fd_2 = accept(listen_fd_2, (struct sockaddr *)&address_1, (socklen_t *)&addrlen)) < 0) {
        perror("[Server] accept() failed on port 2202");
        exit(EXIT_FAILURE);
    }

    int p1_joined = 0;
    int p2_joined = 0;
    int board_width = 0;
    int board_height = 0;
    // Receive and process commands
    while (1) {
        int wrote_to_c1 = 0;
        int wrote_to_c2 = 0;
        if(conn_fd_1 >= 0){
            memset(buffer, 0, BUFFER_SIZE);

            int nbytes = read(conn_fd_1, buffer, BUFFER_SIZE);
            if(buffer[0] == 'F'){
                printf("[Server] Enter message for client1: H 0\n");
                memset(buffer, 0, BUFFER_SIZE);
                send(conn_fd_1, "H 0", 4, 0);
                send(conn_fd_2, "H 1", 4, 0);
                break;
            }

            /*int nbytes = read(conn_fd_1, buffer, BUFFER_SIZE);
            if (nbytes <= 0) {
                perror("[Server] read() failed on port 2201");
                exit(EXIT_FAILURE);
            }*/

            /*printf("[Server] Received from client1: %s\n", buffer);
            if (strcmp(buffer, "quit") == 0) {
                printf("[Server] Client1 chatter quitting...\n");
                close(conn_fd_1);
                conn_fd_1 = -1;
            }*/

            if(!p1_joined){
                //int nbytes = read(conn_fd_1, buffer, BUFFER_SIZE);
                char start, trash;
                int result = sscanf(buffer, " %c %d %d %c", &start, &board_width, &board_height, &trash);
                if(result == 3 && start == 'B' && board_width >= 10 && board_height >= 10){
                    p1_joined = 1;
                    printf("[Server] Enter message for client1: A\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_1, "A", 2, 0);
                    wrote_to_c1 = 1;
                }
                else if(start != 'B'){
                    printf("[Server] Enter message for client1: E 100\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_1, "E 100", 6, 0);
                    continue;
                }
                else{
                    printf("[Server] Enter message for client1: E 200\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_1, "E 200", 6, 0);
                    continue;
                }
            }
            
            
            /*if(p1_joined && p2_joined){
                int nbytes = read(conn_fd_1, buffer, BUFFER_SIZE);
                if(buffer[0] == 'F'){
                    printf("[Server] Enter message for client1: H 0\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_1, "H 0", 4, 0);
                    send(conn_fd_2, "H 1", 4, 0);
                    break;
                }
            }*/
            
            if(!wrote_to_c1){
                printf("[Server] Enter message for client1: response\n");
                memset(buffer, 0, BUFFER_SIZE);
                //fgets(buffer, BUFFER_SIZE, stdin);
                //buffer[strlen(buffer)-1] = '\0';
                send(conn_fd_1, "buffer", 7, 0);
           }

        }

        if(conn_fd_2 >= 0){
            memset(buffer, 0, BUFFER_SIZE);

            int nbytes = read(conn_fd_2, buffer, BUFFER_SIZE);
                if(buffer[0] == 'F'){
                    printf("[Server] Enter message for client2: H 0\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, "H 0", 4, 0);
                    send(conn_fd_1, "H 1", 4, 0);
                    break;
                }

            /*int nbytes = read(conn_fd_2, buffer, BUFFER_SIZE);
            if (nbytes <= 0) {
                perror("[Server] read() failed on port 2202");
                exit(EXIT_FAILURE);
            }*/

            /*printf("[Server] Received from client2: %s\n", buffer);
            if (strcmp(buffer, "quit") == 0) {
                printf("[Server] Client2 chatter quitting...\n");
                close(conn_fd_2);
                conn_fd_2 = -1;
            }*/

            if(!p2_joined){
                //int nbytes = read(conn_fd_2, buffer, BUFFER_SIZE);
                //char start, trash;
                char start, trash;
                int result = sscanf(buffer, " %c %c", &start, &trash);
                if(result == 1 && start == 'B'){
                    p2_joined = 1;
                    printf("[Server] Enter message for client2: A\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, "A", 2, 0);
                    wrote_to_c2 = 1;
                }
                else if(start != 'B'){
                    printf("[Server] Enter message for client1: E 100\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, "E 100", 6, 0);
                    continue;
                }
                else{
                    printf("[Server] Enter message for client2: E 200\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, "E 200", 6, 0);
                    continue;
                }
            }
            /*if(p1_joined && p2_joined){
                int nbytes = read(conn_fd_2, buffer, BUFFER_SIZE);
                if(buffer[0] == 'F'){
                    printf("[Server] Enter message for client2: H 0\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, "H 0", 4, 0);
                    send(conn_fd_1, "H 1", 4, 0);
                    break;
                }
            }*/

            if(!wrote_to_c2){
                printf("[Server] Enter message for client2: response\n");
                memset(buffer, 0, BUFFER_SIZE);
                //fgets(buffer, BUFFER_SIZE, stdin);
                //buffer[strlen(buffer)-1] = '\0';
                send(conn_fd_2, "buffer", 7, 0);
            }
        }
        

        if(conn_fd_1 == -1 && conn_fd_2 == -1){
            printf("All clients disconnecting. Closing server...");
            break;
        }
    }

    printf("[Server] Shutting down.\n");
    close(conn_fd_1);
    close(conn_fd_2);
    close(listen_fd_1);
    close(listen_fd_2);
    return EXIT_SUCCESS;
}