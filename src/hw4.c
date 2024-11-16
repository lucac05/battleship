#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define PORT_1 2201
#define PORT_2 2202
#define BUFFER_SIZE 1024
#define min(a, b) ((a) < (b) ? (a) : (b))

typedef struct Piece{
    int my_number;
    int type;
    int rotation;
    int start_col;
    int start_row;
} Piece;

typedef struct Player{
    int **board;
    Piece my_pieces[5];
} Player;


int glbl_height, glbl_width;

int check_fit(int check_row, int check_col){
    if(check_row >= glbl_height || check_row < 0 || check_col >= glbl_width || check_col < 0)
        return 0;
    return 1;
}

int check_available(int **board, int row, int col){
    return board[row][col] == 0;
}


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
    int p1_init = 0;
    int p2_init = 0;
    int board_width = 0;
    int board_height = 0;
    // Receive and process commands
    while (1) {
        int wrote_to_c1 = 0;
        int wrote_to_c2 = 0;
        if(conn_fd_1 >= 0){
            memset(buffer, 0, BUFFER_SIZE);

            int nbytes = read(conn_fd_1, buffer, BUFFER_SIZE);
            
            char start, trash;
            int result = sscanf(buffer, " %c %c", &start, &trash);
            if(result == 1 && start == 'F'){
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

                    glbl_height = board_height;
                    glbl_width = board_width;
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
            if(p2_joined && !p1_init){
                char start, trash;
                int numbers[20] = {-1};
                int result = sscanf(buffer, " %c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %c",
                    &start,
                    &numbers[0], &numbers[1], &numbers[2], &numbers[3], 
                    &numbers[4], &numbers[5], &numbers[6], &numbers[7], 
                    &numbers[8], &numbers[9], &numbers[10], &numbers[11], 
                    &numbers[12], &numbers[13], &numbers[14], &numbers[15], 
                    &numbers[16], &numbers[17], &numbers[18], &numbers[19],
                    &trash);
                
                int err = 9999999;
                if(start != 'I')
                    err = 101;
                if(result - 1 != 20)
                    err = min(err, 201);

                for(int i = 0; i < 20; i++){
                    int cur_num = numbers[i];
                    if(i % 4 == 0){
                        if(cur_num < 1 || cur_num > 7)//shape out of range
                            err = min(err, 300);
                    }
                    else if(i % 4 == 1){
                        if(cur_num < 1 || cur_num > 4)//rotation out of range
                            err = min(err, 301);
                    }
                    else if(i % 4 == 2){
                        if(cur_num >= board_width)//ship doesn't fit in board (column issue)
                            err = min(err, 302);
                    }
                    else{
                        if(cur_num >= board_height)//ship doesn't fit in board (row issue)
                            err = min(err, 302);
                    }
                    if(cur_num < 0)
                        err = min(err, 201);//is this the right error code for a negative input?
                }

                /*
                
                typedef struct Piece{
                    int my_number;
                    int type;
                    int rotation;
                    int start_col;
                    int start_row;
                } Piece;

                typedef struct Player{
                    int **board;
                    Piece my_pieces[5];//only need 5 tho
                } Player;
                
                
                */


                if(err == 9999999){//error code BEFORE CHECKING 302 & 303

                    Player *p1 = malloc(sizeof(Player));//FREE
                    p1->board = malloc(sizeof(int *) * board_height);//FREE
                    for(int i = 0; i < board_height; i++)
                        p1->board[i] = calloc(board_width, sizeof(int));//FREE

                    Piece p1_piece_1 = {1, numbers[0], numbers[1], numbers[2], numbers[3]};
                    Piece p1_piece_2 = {2, numbers[4], numbers[5], numbers[6], numbers[7]};
                    Piece p1_piece_3 = {3, numbers[8], numbers[9], numbers[10], numbers[11]};
                    Piece p1_piece_4 = {4, numbers[12], numbers[13], numbers[14], numbers[15]};
                    Piece p1_piece_5 = {5, numbers[16], numbers[17], numbers[18], numbers[19]};
                    Piece p1_pieces[5] = {p1_piece_1, p1_piece_2, p1_piece_3, p1_piece_4, p1_piece_5};
                    for(int i = 0; i < 5; i++)
                        p1->my_pieces[i] = p1_pieces[i];
                    
                    //setup_board(numbers, p1->board);
                    for(int i = 0; i < 5; i++){
                        Piece cur = p1->my_pieces[i];
                        int row_index = cur.start_row;
                        int col_index = cur.start_col;

                        if(cur.type == 1){//square
                            if(row_index + 1 >= board_height || cur.start_col + 1 >= board_width){//square goes out of bounds
                                err = min(err, 302); 
                            }
                            //checking for overlap
                            if(p1->board[row_index][col_index] != 0 || p1->board[row_index][col_index + 1] != 0 || p1->board[row_index + 1][col_index] != 0 ||p1->board[row_index + 1][col_index + 1] != 0){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            p1->board[row_index][col_index] = cur.my_number;
                            p1->board[row_index][col_index + 1] = cur.my_number;
                            p1->board[row_index + 1][col_index] = cur.my_number;
                            p1->board[row_index + 1][col_index + 1] = cur.my_number;
                        }
                        else if(cur.type == 2){//line
                            if(((cur.rotation == 1 || cur.rotation == 3) && row_index + 3 >= board_height) || ((cur.rotation == 2 || cur.rotation == 4) && col_index + 3 >= board_width)){//line goes out of boudns
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (p1->board[row_index][col_index] != 0 || p1->board[row_index + 1][col_index] != 0 || p1->board[row_index + 2][col_index] != 0 || p1->board[row_index + 3][col_index] != 0)) || ((cur.rotation == 2 || cur.rotation == 4) && (p1->board[row_index][col_index] != 0 || p1->board[row_index][col_index + 1] != 0 || p1->board[row_index][col_index + 2] != 0 || p1->board[row_index][col_index + 3] != 0))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                                p1->board[row_index + 3][col_index] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                                p1->board[row_index][col_index + 3] = cur.my_number;
                            }
                        }
                        else if(cur.type == 3){//3_zig
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index - 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index + 1)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                        }
                        else if(cur.type == 4){//L
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index) || !check_fit(row_index + 2, col_index + 1))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index + 1))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index - 1, col_index + 2)))){//L goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 2, col_index) || !check_available(p1->board, row_index + 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index + 1))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2) || !check_available(p1->board, row_index - 1, col_index + 2)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                                p1->board[row_index - 1][col_index + 2] = cur.my_number;
                            }
                        }
                        else if(cur.type == 5){//5_zig
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                            }
                        }
                        else if(cur.type == 6){//new-L
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index - 2, col_index + 1))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 1, col_index + 2))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index + 1, col_index + 2)))){//L goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index - 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 2))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 2, col_index))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2) || !check_available(p1->board, row_index + 1, col_index + 2)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index - 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                                p1->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                        }
                        else if(cur.type == 7){//T
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index + 1, col_index + 1))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index)))){//L goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                            }
                        }
                        

                       
                    }

                }
                if(err != 9999999){//error code AFTER CHECKING ALL POSSIBEL ONES (incl. 302 & 303)
                    //yes error(s)
                    char tmp_str[999];
                    snprintf(tmp_str, sizeof(tmp_str), "%d", err);

                    char err_str[999];
                    strcpy(err_str, "E ");
                    strcat(err_str, tmp_str);

                    printf("[Server] Enter message for client1: %s\n", err_str);
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_1, err_str, sizeof(err_str), 0);
                    continue;
                }
                //else{
                    p1_init = 1;
                    /*printf("[Server] Enter message for client1: A\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_1, "A", 2, 0);
                    wrote_to_c1 = 1;
                }*/
                
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
            char start, trash;
            int result = sscanf(buffer, " %c %c", &start, &trash);
            if(result == 1 && start == 'F'){
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
                    printf("[Server] Enter message for client2: E 200\n");//should be E 100 ?
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, "E 200", 6, 0);
                    continue;
                }
            }
            
            if(p2_joined && p1_init && !p2_init){
                char start, trash;
                int numbers[20] = {-1};
                int result = sscanf(buffer, " %c %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %c",
                    &start,
                    &numbers[0], &numbers[1], &numbers[2], &numbers[3], 
                    &numbers[4], &numbers[5], &numbers[6], &numbers[7], 
                    &numbers[8], &numbers[9], &numbers[10], &numbers[11], 
                    &numbers[12], &numbers[13], &numbers[14], &numbers[15], 
                    &numbers[16], &numbers[17], &numbers[18], &numbers[19],
                    &trash);
                
                int err = 9999999;
                if(start != 'I')
                    err = 101;
                if(result - 1 != 20)
                    err = min(err, 201);

                for(int i = 0; i < 20; i++){
                    int cur_num = numbers[i];
                    if(i % 4 == 0){
                        if(cur_num < 1 || cur_num > 7)//shape out of range
                            err = min(err, 300);
                    }
                    else if(i % 4 == 1){
                        if(cur_num < 1 || cur_num > 4)//rotation out of range
                            err = min(err, 301);
                    }
                    else if(i % 4 == 2){
                        if(cur_num >= board_width)//ship doesn't fit in board (column issue)
                            err = min(err, 302);
                    }
                    else{
                        if(cur_num >= board_height)//ship doesn't fit in board (row issue)
                            err = min(err, 302);
                    }
                    if(cur_num < 0)
                        err = min(err, 201);//is this the right error code for a negative input?
                }

                /*
                
                typedef struct Piece{
                    int my_number;
                    int type;
                    int rotation;
                    int start_col;
                    int start_row;
                } Piece;

                typedef struct Player{
                    int **board;
                    Piece my_pieces[5];//only need 5 tho
                } Player;
                
                
                */


                if(err == 9999999){//error code BEFORE CHECKING 302 & 303

                    Player *p1 = malloc(sizeof(Player));//FREE
                    p1->board = malloc(sizeof(int *) * board_height);//FREE
                    for(int i = 0; i < board_height; i++)
                        p1->board[i] = calloc(board_width, sizeof(int));//FREE

                    Piece p1_piece_1 = {1, numbers[0], numbers[1], numbers[2], numbers[3]};
                    Piece p1_piece_2 = {2, numbers[4], numbers[5], numbers[6], numbers[7]};
                    Piece p1_piece_3 = {3, numbers[8], numbers[9], numbers[10], numbers[11]};
                    Piece p1_piece_4 = {4, numbers[12], numbers[13], numbers[14], numbers[15]};
                    Piece p1_piece_5 = {5, numbers[16], numbers[17], numbers[18], numbers[19]};
                    Piece p1_pieces[5] = {p1_piece_1, p1_piece_2, p1_piece_3, p1_piece_4, p1_piece_5};
                    for(int i = 0; i < 5; i++)
                        p1->my_pieces[i] = p1_pieces[i];
                    
                    //setup_board(numbers, p1->board);
                    for(int i = 0; i < 5; i++){
                        Piece cur = p1->my_pieces[i];
                        int row_index = cur.start_row;
                        int col_index = cur.start_col;

                        if(cur.type == 1){//square
                            if(row_index + 1 >= board_height || cur.start_col + 1 >= board_width){//square goes out of bounds
                                err = min(err, 302); 
                            }
                            //checking for overlap
                            if(p1->board[row_index][col_index] != 0 || p1->board[row_index][col_index + 1] != 0 || p1->board[row_index + 1][col_index] != 0 ||p1->board[row_index + 1][col_index + 1] != 0){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            p1->board[row_index][col_index] = cur.my_number;
                            p1->board[row_index][col_index + 1] = cur.my_number;
                            p1->board[row_index + 1][col_index] = cur.my_number;
                            p1->board[row_index + 1][col_index + 1] = cur.my_number;
                        }
                        else if(cur.type == 2){//line
                            if(((cur.rotation == 1 || cur.rotation == 3) && row_index + 3 >= board_height) || ((cur.rotation == 2 || cur.rotation == 4) && col_index + 3 >= board_width)){//line goes out of boudns
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (p1->board[row_index][col_index] != 0 || p1->board[row_index + 1][col_index] != 0 || p1->board[row_index + 2][col_index] != 0 || p1->board[row_index + 3][col_index] != 0)) || ((cur.rotation == 2 || cur.rotation == 4) && (p1->board[row_index][col_index] != 0 || p1->board[row_index][col_index + 1] != 0 || p1->board[row_index][col_index + 2] != 0 || p1->board[row_index][col_index + 3] != 0))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                                p1->board[row_index + 3][col_index] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                                p1->board[row_index][col_index + 3] = cur.my_number;
                            }
                        }
                        else if(cur.type == 3){//3_zig
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index - 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index + 1)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                        }
                        else if(cur.type == 4){//L
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index) || !check_fit(row_index + 2, col_index + 1))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index + 1))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index - 1, col_index + 2)))){//L goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 2, col_index) || !check_available(p1->board, row_index + 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index + 1))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2) || !check_available(p1->board, row_index - 1, col_index + 2)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                                p1->board[row_index - 1][col_index + 2] = cur.my_number;
                            }
                        }
                        else if(cur.type == 5){//5_zig
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                            }
                        }
                        else if(cur.type == 6){//new-L
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index - 2, col_index + 1))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 1, col_index + 2))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index + 1, col_index + 2)))){//L goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index - 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 2))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 2, col_index))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2) || !check_available(p1->board, row_index + 1, col_index + 2)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index - 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                                p1->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                        }
                        else if(cur.type == 7){//T
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index + 1, col_index + 1))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index)))){//L goes out of boudds
                                err = min(err, 302);
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index)))){
                                err = min(err, 303);
                                break;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index][col_index + 1] = cur.my_number;
                                p1->board[row_index - 1][col_index + 1] = cur.my_number;
                                p1->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p1->board[row_index][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index] = cur.my_number;
                                p1->board[row_index + 1][col_index + 1] = cur.my_number;
                                p1->board[row_index + 2][col_index] = cur.my_number;
                            }
                        }
                        

                       
                    }

                }
                if(err != 9999999){//error code AFTER CHECKING ALL POSSIBEL ONES (incl. 302 & 303)
                    //yes error(s)
                    char tmp_str[999];
                    snprintf(tmp_str, sizeof(tmp_str), "%d", err);

                    char err_str[999];
                    strcpy(err_str, "E ");
                    strcat(err_str, tmp_str);

                    printf("[Server] Enter message for client1: %s\n", err_str);
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, err_str, sizeof(err_str), 0);
                    continue;
                }
                //else{
                    p2_init = 1;
                    /*printf("[Server] Enter message for client2: A\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    send(conn_fd_2, "A", 2, 0);
                    wrote_to_c2 = 1;
                }*/
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