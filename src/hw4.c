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

/*typedef struct Shot{
    int hit;//1 if hit, -1 if miss, 0 upon initialization
} Shot;*/

typedef struct Player{
    int **board;
    Piece my_pieces[5];
    int **my_shots;//FOR EACH ENTRY: 1 if hit, -1 if miss, 0 upon initialization
    int num_ships;
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

int check_sunk_ship(int **board, int target){
    int count = 0;
    for(int r = 0; r < glbl_height; r++){
        for(int c = 0; c < glbl_width; c++){
            if(board[r][c] == target)
                count++;
        }
    }
    if(count == 4)
        return 1;//ship sunk (all 4 pieces hit)
    return 0;//ship not sunk
    
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


    //socket 2
    address_2.sin_family = AF_INET;
    address_2.sin_addr.s_addr = INADDR_ANY;
    address_2.sin_port = htons(PORT_2);




    if (bind(listen_fd_1, (struct sockaddr *)&address_1, sizeof(address_1)) < 0) {
        perror("[Server] bind() failed on port 2201");
        exit(EXIT_FAILURE);
    }

    

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


    int wrote_to_c1 = 0;
    int wrote_to_c2 = 0;
    int read_from_c1 = 1;

    int p1_forfeited = 0;
    int p2_forfeited = 0;

    int p1_shot = 0;
    int p2_shot = 0;

    Player *p1 = NULL;
    Player *p2 = NULL;
    // Receive and process commands
    while (1) {
        
        if(conn_fd_1 >= 0 && read_from_c1){
            memset(buffer, 0, BUFFER_SIZE);

            int nbytes = read(conn_fd_1, buffer, BUFFER_SIZE);
            if (nbytes <= 0) {
                perror("[Server] read() failed on port 2201");
                //should this be here?
                close(conn_fd_1);
                close(conn_fd_2);
                close(listen_fd_1);
                close(listen_fd_2);
                exit(EXIT_FAILURE);
            }

            if(p2_forfeited){
                printf("[Server] Enter message for client1: H 1\n");
                if(send(conn_fd_1, "H 1", 4, 0) < 0)
                    perror("[Server] Failed to send packet to player.");
                break;
            }
            char start, trash;
            int result = sscanf(buffer, " %c %c", &start, &trash);
            if(result == 1 && start == 'F'){
                p1_forfeited = 1;
                printf("[Server] Enter message for client1: H 0\n");
                memset(buffer, 0, BUFFER_SIZE);
                if(send(conn_fd_1, "H 0", 4, 0) < 0)
                    perror("[Server] Failed to send halt packet to forfeiting player.");
                wrote_to_c1 = 1;
                //send(conn_fd_2, "H 1", 4, 0);
                
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

            if(!p1_joined && !p1_forfeited){
                //int nbytes = read(conn_fd_1, buffer, BUFFER_SIZE);
                char start, trash;
                int result = sscanf(buffer, " %c %d %d %c", &start, &board_width, &board_height, &trash);
                if(result == 3 && start == 'B' && board_width >= 10 && board_height >= 10){
                    p1_joined = 1;
                    printf("[Server] Enter message for client1: A\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    
                    if(send(conn_fd_1, "A", 2, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    wrote_to_c1 = 1;

                    glbl_height = board_height;
                    glbl_width = board_width;
                }
                else if(start != 'B'){
                    printf("[Server] Enter message for client1: E 100\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    if(send(conn_fd_1, "E 100", 6, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    
                    continue;
                }
                else{
                    printf("[Server] Enter message for client1: E 200\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    
                    if(send(conn_fd_1, "E 200", 6, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    
                    continue;
                }
            }
            if(p2_joined && !p1_init && !p1_forfeited){
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

                    p1 = calloc(1, sizeof(Player));//FREE
                    p1->board = calloc(glbl_height, sizeof(int *));//FREE
                    for(int i = 0; i < glbl_height; i++)
                        p1->board[i] = calloc(glbl_width, sizeof(int));//FREE

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
                            if(!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1)){//square goes out of bounds
                                err = min(err, 302);
                                continue;
                            }
                            //checking for overlap
                            if(!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1)){//square goes out of bounds
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            p1->board[row_index][col_index] = cur.my_number;
                            p1->board[row_index][col_index + 1] = cur.my_number;
                            p1->board[row_index + 1][col_index] = cur.my_number;
                            p1->board[row_index + 1][col_index + 1] = cur.my_number;
                        }
                        else if(cur.type == 2){//line
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index) || !check_fit(row_index + 3, col_index))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index, col_index + 3)))){//line goes out of boudns
                                err = min(err, 302);
                                continue;//CHANGED THIS
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 2, col_index) || !check_available(p1->board, row_index + 3, col_index))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2) || !check_available(p1->board, row_index, col_index + 3)))){//line goes out of boudns
                                err = min(err, 303);
                                continue;//now what?
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
                                continue;
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index + 1)))){
                                err = min(err, 303);
                                continue;//now what?
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
                                continue;
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 2, col_index) || !check_available(p1->board, row_index + 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index + 1))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2) || !check_available(p1->board, row_index - 1, col_index + 2)))){
                                err = min(err, 303);
                                continue;//now what?
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
                                continue;
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 303);
                                continue;//now what?
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
                                continue;
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index - 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 2))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 2, col_index))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index, col_index + 2) || !check_available(p1->board, row_index + 1, col_index + 2)))){
                                err = min(err, 303);
                                continue;//now what?
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
                                continue;
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 2 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index + 1, col_index + 1))) || (cur.rotation == 3 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index, col_index + 1) || !check_available(p1->board, row_index - 1, col_index + 1) || !check_available(p1->board, row_index, col_index + 2))) || (cur.rotation == 4 && (!check_available(p1->board, row_index, col_index) || !check_available(p1->board, row_index + 1, col_index) || !check_available(p1->board, row_index + 1, col_index + 1) || !check_available(p1->board, row_index + 2, col_index)))){
                                err = min(err, 303);
                                continue;//now what?
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
                    char tmp_str[999] = {0};
                    snprintf(tmp_str, sizeof(tmp_str), "%d", err);

                    char err_str[999] = {0};
                    strcpy(err_str, "E ");
                    strcat(err_str, tmp_str);

                    printf("[Server] Enter message for client1: %s\n", err_str);
                    memset(buffer, 0, BUFFER_SIZE);
                    
 
                    if(send(conn_fd_1, err_str, strlen(err_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    


                    /*for(int i = 0; i < glbl_height; i++)
                        free(p1->board[i]);
                    free(p1->board);
                    free(p1);
                    p1 = NULL;*/

                    for(int i = 0; i < glbl_height; i++){
                        if(p1 && p1->board && p1->board[i])
                            free(p1->board[i]);
                    }
                    if(p1 && p1->board)
                        free(p1->board);
                    if(p1)
                        free(p1);
                    p1 = NULL;




                    continue;
                    //REMEMBER TO FREE HERE TO ADDRESS MULTIPLE MALLOC comment COMMENT
                }
                else{
                    //char test[100];
                    //sprintf(test, "%d %d", glbl_height, glbl_width);
                    //send(conn_fd_1, test, sizeof(test), 0);//LEONARDO DA VINCI

                    p1->my_shots = calloc(glbl_height, sizeof(int *));//FREE
                    for(int i = 0; i < board_height; i++)
                        p1->my_shots[i] = calloc(glbl_width, sizeof(int));//FREE
                    
                    p1->num_ships = 5;
                    p1_init = 1;
                    printf("[Server] Enter message for client1: A\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    if(send(conn_fd_1, "A", 2, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    
                    wrote_to_c1 = 1;
                }
                
            }
            if(p1_init && p2_init && !p1_forfeited && !p1_shot){
                int row_shot, col_shot;
                char start, trash;
                int result = sscanf(buffer, " %c %d %d %c", &start, &row_shot, &col_shot, &trash);
                int err = 9999999;
                
                if(start != 'S' && start != 'Q')
                    err = min(err, 102);//invalid packet type
                
                if((start == 'S' && result != 3) || (start == 'Q' && result != 1))
                    err = min(err, 202);//invalid number of parameters (assuming S-type)
                
                if(start == 'S' && result == 3 && !(row_shot >= 0 && row_shot < glbl_height && col_shot >= 0 && col_shot < glbl_width))
                    err = min(err, 400);//cell not in game board (assuming S-type)
                else if(start == 'S' && result == 3 && p1->my_shots[row_shot][col_shot])
                    err = min(err, 401);//cell already guessed (assuming S-type)
                
                if(err != 9999999){//there was an error
                    //if(err == 303)
                    //    send(conn_fd_1, buffer, strlen(buffer), 0);//EZIO AUDITORE
                    char tmp_str[999] = {0};
                    snprintf(tmp_str, sizeof(tmp_str), "%d", err);

                    char err_str[999] = {0};
                    strcpy(err_str, "E ");
                    strcat(err_str, tmp_str);

                    printf("[Server] Enter message for client1: %s\n", err_str);
                    memset(buffer, 0, BUFFER_SIZE);

                    if(send(conn_fd_1, err_str, strlen(err_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    continue;
                    
                }
                //p1_shot = 1;
                //p2_shot = 0;moved inside 'S' block
                
                //handling S
                if(start == 'S'){
                    p1_shot = 1;
                    p2_shot = 0;
                    int hit = p2->board[row_shot][col_shot];
                    if(hit){
                        p1->my_shots[row_shot][col_shot] = 1;//remember the hit as (1)
                        p2->board[row_shot][col_shot] *= -1;//mark as a hit by negating
                        int ship_sunk = check_sunk_ship(p2->board, p2->board[row_shot][col_shot]);//search for the same negative number
                        if(ship_sunk)
                            p2->num_ships--;//need to handle case of sinking all ships
                    }
                    else{
                        p1->my_shots[row_shot][col_shot] = -1;//remember the miss as (-1)
                        p2->board[row_shot][col_shot] = 9;//mark as a miss by setting to 9 (water, not a ship)
                    }
                    char printout[3] = {' ', hit ? 'H' : 'M', '\0'};
                    
                    
                    
                    char tmp_str[999] = {0};
                    snprintf(tmp_str, sizeof(tmp_str), "%d", p2->num_ships);

                    char msg_str[999] = {0};
                    strcpy(msg_str, "R ");
                    strcat(msg_str, tmp_str);
                    strcat(msg_str, printout);

                    printf("[Server] Enter message for client1: %s\n", msg_str);

                    if(send(conn_fd_1, msg_str, strlen(msg_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    
                    memset(buffer, 0, BUFFER_SIZE);

                    wrote_to_c1 = 1;

                    //VERY LIKELY WRONG
                    if(p2->num_ships == 0){
                        if(send(conn_fd_2, "H 0", 4, 0) < 0)
                            perror("[Server] Failed to send packet to player.");
                        
                        //need to read from p1 in between
                        p2_forfeited = 1;
                        continue;
                        //send(conn_fd_1, "H 1", 4, 0);
                        //break;
                    } 
                }
                else if(start == 'Q'){
                    //dont check for extraneous params because there's no error message for it
                    char helper_str[99] = {0};
                    snprintf(helper_str, sizeof(helper_str), "%d ", p2->num_ships);
                    char msg_str[999] = {0};
                    strcpy(msg_str, "G ");
                    strcat(msg_str, helper_str);
                    printf("CUR_MSG: %s\n", msg_str);

                    for(int r = 0; r < glbl_height; r++){
                        for(int c = 0; c < glbl_width; c++){
                            if(p1->my_shots[r][c] == 0)
                                continue;
                            int hit_at_pos = p1->my_shots[r][c] == 1;//1 if hit, -1 if miss
                            char printout[3] = {hit_at_pos ? 'H' : 'M', ' '};
                            strcat(msg_str, printout);
                            memset(helper_str, 0, sizeof(helper_str));
                            snprintf(helper_str, sizeof(helper_str), "%d %d ", c, r);
                            strcat(msg_str, helper_str);
                        }
                    }
                    
                    if(msg_str[strlen(msg_str) - 1] == ' ')
                        msg_str[strlen(msg_str) - 1] = '\0';
                    else
                        msg_str[strlen(msg_str)] = '\0';
                    printf("[Server] Enter message for client1: %s\n", msg_str);

                    if(send(conn_fd_1, msg_str, strlen(msg_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    
                    memset(buffer, 0, BUFFER_SIZE);
                    continue;//still p1's turn so skip p2

                }
                else{
                    for(int i = 0; i < 1000; i++)
                        printf("How did we get here?");
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
                if(send(conn_fd_1, "buffer", 7, 0) < 0)
                    perror("[Server] Failed to send packet to player.");
                
           }

        }

        if(conn_fd_2 >= 0){
            memset(buffer, 0, BUFFER_SIZE);

            int nbytes = read(conn_fd_2, buffer, BUFFER_SIZE);
            if (nbytes <= 0) {
                perror("[Server] read() failed on port 2202");
                close(conn_fd_1);
                close(conn_fd_2);
                close(listen_fd_1);
                close(listen_fd_2);
                exit(EXIT_FAILURE);
            }

            if(p1_forfeited){
                printf("[Server] Enter message for client2: H 1\n");
                if(send(conn_fd_2, "H 1", 4, 0) < 0)
                    perror("[Server] Failed to send packet to player.");
                
                    
                break;
            }
            char start, trash;
            int result = sscanf(buffer, " %c %c", &start, &trash);
            if(result == 1 && start == 'F'){
                read_from_c1 = 1;
                p2_forfeited = 1;
                printf("[Server] Enter message for client2: H 0\n");
                memset(buffer, 0, BUFFER_SIZE);
                if(send(conn_fd_2, "H 0", 4, 0) < 0)
                    perror("[Server] Failed to send halt packet to forfeiting player.");
                //send(conn_fd_1, "H 1", 4, 0);//removed
                continue;
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
                    
                    if(send(conn_fd_2, "A", 2, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    wrote_to_c2 = 1;

                    read_from_c1 = 1;

                }
                else if(start != 'B'){
                    printf("[Server] Enter message for client1: E 100\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    

                    if(send(conn_fd_2, "E 100", 6, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    
                    read_from_c1 = 0;

                    continue;
                }
                else{
                    //char test[100];
                    //sprintf(test, "%d %d", glbl_height, glbl_width);
                    //send(conn_fd_2, test, sizeof(test), 0);//LEONARDO DA VINCI

                    printf("[Server] Enter message for client2: E 200\n");//should be E 100 ? new comment
                    memset(buffer, 0, BUFFER_SIZE);
                    if(send(conn_fd_2, "E 200", 6, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    

                    read_from_c1 = 0;

                    continue;
                }
            }
            
            if(p2_joined && p1_init && !p2_init /* && !wrote_to_c2*/){
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

                    p2 = calloc(1, sizeof(Player));//FREE
                    p2->board = calloc(glbl_height, sizeof(int *));//FREE
                    for(int i = 0; i < glbl_height; i++)
                        p2->board[i] = calloc(glbl_width, sizeof(int));//FREE

                    Piece p2_piece_1 = {1, numbers[0], numbers[1], numbers[2], numbers[3]};
                    Piece p2_piece_2 = {2, numbers[4], numbers[5], numbers[6], numbers[7]};
                    Piece p2_piece_3 = {3, numbers[8], numbers[9], numbers[10], numbers[11]};
                    Piece p2_piece_4 = {4, numbers[12], numbers[13], numbers[14], numbers[15]};
                    Piece p2_piece_5 = {5, numbers[16], numbers[17], numbers[18], numbers[19]};
                    Piece p2_pieces[5] = {p2_piece_1, p2_piece_2, p2_piece_3, p2_piece_4, p2_piece_5};
                    for(int i = 0; i < 5; i++)
                        p2->my_pieces[i] = p2_pieces[i];
                    
                    //setup_board(numbers, p2->board);
                    for(int i = 0; i < 5; i++){
                        Piece cur = p2->my_pieces[i];
                        int row_index = cur.start_row;
                        int col_index = cur.start_col;

                        if(cur.type == 1){//square
                            if(!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1)){//square goes out of bounds
                                err = min(err, 302); 
                                continue;
                            }
                            //checking for overlap
                            if(!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index + 1, col_index + 1)){//square goes out of bounds
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            p2->board[row_index][col_index] = cur.my_number;
                            p2->board[row_index][col_index + 1] = cur.my_number;
                            p2->board[row_index + 1][col_index] = cur.my_number;
                            p2->board[row_index + 1][col_index + 1] = cur.my_number;
                        }
                        else if(cur.type == 2){//line
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index) || !check_fit(row_index + 3, col_index))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index, col_index + 3)))){//line goes out of boudns
                                err = min(err, 302);
                                continue;
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index + 2, col_index) || !check_available(p2->board, row_index + 3, col_index))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index, col_index + 2) || !check_available(p2->board, row_index, col_index + 3)))){//line goes out of boudns
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index + 2][col_index] = cur.my_number;
                                p2->board[row_index + 3][col_index] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index][col_index + 2] = cur.my_number;
                                p2->board[row_index][col_index + 3] = cur.my_number;
                            }
                        }
                        else if(cur.type == 3){//3_zig
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index - 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 302);
                                continue;
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index - 1, col_index + 1) || !check_available(p2->board, row_index - 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index + 1, col_index + 1) || !check_available(p2->board, row_index + 2, col_index + 1)))){
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index - 1][col_index + 1] = cur.my_number;
                                p2->board[row_index - 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index + 1] = cur.my_number;
                                p2->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                        }
                        else if(cur.type == 4){//L
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index) || !check_fit(row_index + 2, col_index + 1))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index + 1))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index - 1, col_index + 2)))){//L goes out of boudds
                                err = min(err, 302);
                                continue;
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index + 2, col_index) || !check_available(p2->board, row_index + 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index, col_index + 2))) || (cur.rotation == 3 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index + 1, col_index + 1) || !check_available(p2->board, row_index + 2, col_index + 1))) || (cur.rotation == 4 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index, col_index + 2) || !check_available(p2->board, row_index - 1, col_index + 2)))){
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index + 2][col_index] = cur.my_number;
                                p2->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index + 1][col_index + 1] = cur.my_number;
                                p2->board[row_index + 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index][col_index + 2] = cur.my_number;
                                p2->board[row_index - 1][col_index + 2] = cur.my_number;
                            }
                        }
                        else if(cur.type == 5){//5_zig
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 302);
                                continue;
                            }
                            //checking for overlap
                            if(((cur.rotation == 1 || cur.rotation == 3) && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index + 1, col_index + 1) || !check_available(p2->board, row_index + 1, col_index + 2))) || ((cur.rotation == 2 || cur.rotation == 4) && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index - 1, col_index + 1)))){//zig goes out of boudds
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1 || cur.rotation == 3){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index + 1][col_index + 1] = cur.my_number;
                                p2->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2 || cur.rotation == 4){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index - 1][col_index + 1] = cur.my_number;
                            }
                        }
                        else if(cur.type == 6){//new-L
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index - 2, col_index + 1))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 1, col_index + 2))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 2, col_index))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index, col_index + 2) || !check_fit(row_index + 1, col_index + 2)))){//L goes out of boudds
                                err = min(err, 302);
                                continue;
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index - 1, col_index + 1) || !check_available(p2->board, row_index - 2, col_index + 1))) || (cur.rotation == 2 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index + 1, col_index + 1) || !check_available(p2->board, row_index + 1, col_index + 2))) || (cur.rotation == 3 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index + 2, col_index))) || (cur.rotation == 4 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index, col_index + 2) || !check_available(p2->board, row_index + 1, col_index + 2)))){
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index - 1][col_index + 1] = cur.my_number;
                                p2->board[row_index - 2][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index + 1] = cur.my_number;
                                p2->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index + 2][col_index] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index][col_index + 2] = cur.my_number;
                                p2->board[row_index + 1][col_index + 2] = cur.my_number;
                            }
                        }
                        else if(cur.type == 7){//T
                            if((cur.rotation == 1 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 2 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index + 1, col_index + 1))) || (cur.rotation == 3 && (!check_fit(row_index, col_index) || !check_fit(row_index, col_index + 1) || !check_fit(row_index - 1, col_index + 1) || !check_fit(row_index, col_index + 2))) || (cur.rotation == 4 && (!check_fit(row_index, col_index) || !check_fit(row_index + 1, col_index) || !check_fit(row_index + 1, col_index + 1) || !check_fit(row_index + 2, col_index)))){//L goes out of boudds
                                err = min(err, 302);
                                continue;
                            }
                            //checking for overlap
                            if((cur.rotation == 1 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index + 1, col_index + 1) || !check_available(p2->board, row_index, col_index + 2))) || (cur.rotation == 2 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index - 1, col_index + 1) || !check_available(p2->board, row_index + 1, col_index + 1))) || (cur.rotation == 3 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index, col_index + 1) || !check_available(p2->board, row_index - 1, col_index + 1) || !check_available(p2->board, row_index, col_index + 2))) || (cur.rotation == 4 && (!check_available(p2->board, row_index, col_index) || !check_available(p2->board, row_index + 1, col_index) || !check_available(p2->board, row_index + 1, col_index + 1) || !check_available(p2->board, row_index + 2, col_index)))){
                                err = min(err, 303);
                                continue;//now what?
                            }
                            //adding the ships to the board after ensuring all is well
                            if(cur.rotation == 1){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index + 1][col_index + 1] = cur.my_number;
                                p2->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 2){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index - 1][col_index + 1] = cur.my_number;
                                p2->board[row_index + 1][col_index + 1] = cur.my_number;
                            }
                            else if(cur.rotation == 3){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index][col_index + 1] = cur.my_number;
                                p2->board[row_index - 1][col_index + 1] = cur.my_number;
                                p2->board[row_index][col_index + 2] = cur.my_number;
                            }
                            else if(cur.rotation == 4){
                                p2->board[row_index][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index] = cur.my_number;
                                p2->board[row_index + 1][col_index + 1] = cur.my_number;
                                p2->board[row_index + 2][col_index] = cur.my_number;
                            }
                        }
                        

                       
                    }

                }
                if(err != 9999999){//error code AFTER CHECKING ALL POSSIBEL ONES (incl. 302 & 303)
                    //yes error(s)
                    //if(err == 303)
                    //    send(conn_fd_2, buffer, strlen(buffer), 0);//EZIO AUDITORE
                    char tmp_str[999] = {0};
                    snprintf(tmp_str, sizeof(tmp_str), "%d", err);

                    char err_str[999] = {0};
                    strcpy(err_str, "E ");
                    strcat(err_str, tmp_str);

                    printf("[Server] Enter message for client1: %s\n", err_str);
                    memset(buffer, 0, BUFFER_SIZE);
                    

                    if(send(conn_fd_2, err_str, strlen(err_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    

                    read_from_c1 = 0;


                    /*for(int i = 0; i < glbl_height; i++)
                        free(p2->board[i]);
                    free(p2->board);
                    free(p2);
                    p2 = NULL;*/

                    for(int i = 0; i < glbl_height; i++){
                        if(p2 && p2->board && p2->board[i])
                            free(p2->board[i]);
                    }
                    if(p2 && p2->board)
                        free(p2->board);
                    if(p2)
                        free(p2);
                    p2 = NULL;

                    continue;
                }
                else{
                    p2->my_shots = calloc(glbl_height, sizeof(int *));//FREE
                    for(int i = 0; i < board_height; i++)
                        p2->my_shots[i] = calloc(glbl_width, sizeof(int));//FREE
                    
                    p2->num_ships = 5;
                    p2_init = 1;
                    printf("[Server] Enter message for client2: A\n");
                    memset(buffer, 0, BUFFER_SIZE);
                    
                    if(send(conn_fd_2, "A", 2, 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    wrote_to_c2 = 1;

                    read_from_c1 = 1;
                }
            }
            
            
            if(p1_init && p2_init && p1_shot && !p2_shot){
                int row_shot, col_shot;
                char start, trash;
                int result = sscanf(buffer, " %c %d %d %c", &start, &row_shot, &col_shot, &trash);
                int err = 9999999;
                printf("HERE ONCE");
                if(start != 'S' && start != 'Q')
                    err = min(err, 102);//invalid packet type
                
                if((start == 'S' && result != 3) || (start == 'Q' && result != 1))
                    err = min(err, 202);//invalid number of parameters (assuming S-type)
                
                if(start == 'S' && result == 3 && !(row_shot >= 0 && row_shot < glbl_height && col_shot >= 0 && col_shot < glbl_width))
                    err = min(err, 400);//cell not in game board (assuming S-type)
                else if(start == 'S' && result == 3 && p2->my_shots[row_shot][col_shot])
                    err = min(err, 401);//cell already guessed (assuming S-type)
                
                printf("CURRENT ERROR %d", err);
                if(err != 9999999){//there was an error
                    printf("ERROR IN BUFFER: %s", buffer);
                    char tmp_str[999] = {0};
                    snprintf(tmp_str, sizeof(tmp_str), "%d", err);

                    char err_str[999] = {0};
                    strcpy(err_str, "E ");
                    strcat(err_str, tmp_str);

                    printf("[Server] Enter message for client2: %s\n", err_str);
                    memset(buffer, 0, BUFFER_SIZE);

                    if(send(conn_fd_2, err_str, strlen(err_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    
                    read_from_c1 = 0;
                    wrote_to_c2 = 0;
                    continue;
                }
                
                
                
                //handling S
                if(start == 'S'){
                    read_from_c1 = 1;//was outside
                    p2_shot = 1;
                    p1_shot = 0;
                    int hit = p1->board[row_shot][col_shot];
                    if(hit){
                        p2->my_shots[row_shot][col_shot] = 1;//remember the hit as (1)
                        p1->board[row_shot][col_shot] *= -1;//mark as a hit by negating
                        int ship_sunk = check_sunk_ship(p1->board, p1->board[row_shot][col_shot]);//search for the same negative number
                        if(ship_sunk)
                            p1->num_ships--;//need to handle case of sinking all ships
                    }
                    else{
                        p2->my_shots[row_shot][col_shot] = -1;//remember the miss as (-1)
                        p1->board[row_shot][col_shot] = 9;//mark as a miss by setting to 9 (water, not a ship)
                    }
                    char printout[3] = {' ', hit ? 'H' : 'M', '\0'};
                    
                    
                    char tmp_str[999] = {0};
                    snprintf(tmp_str, sizeof(tmp_str), "%d", p1->num_ships);

                    char msg_str[999] = {0};
                    strcpy(msg_str, "R ");
                    strcat(msg_str, tmp_str);
                    strcat(msg_str, printout);

                    printf("[Server] Enter message for client2: %s\n", msg_str);
                    
                    
                    
                    if(send(conn_fd_2, msg_str, strlen(msg_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    memset(buffer, 0, BUFFER_SIZE);

                    wrote_to_c2 = 1;

                    //VERY LIKELY WRONG
                    if(p1->num_ships == 0){//p2 won
                        
                        if(send(conn_fd_1, "H 0", 4, 0) < 0)
                            perror("[Server] Failed to send packet to player.");
                        p1_forfeited = 1;
                        read_from_c1 = 0;
                        //send(conn_fd_2, "H 1", 4, 0);
                        //break;
                        continue;
                    } 
                }
                else if(start == 'Q'){
                    //dont check for extraneous params because there's no error message for it
                    char helper_str[99] = {0};
                    snprintf(helper_str, sizeof(helper_str), "%d ", p1->num_ships);
                    char msg_str[999] = {0};
                    strcpy(msg_str, "G ");
                    strcat(msg_str, helper_str);
                    printf("CUR_MSG: %s\n", msg_str);

                    for(int r = 0; r < glbl_height; r++){
                        for(int c = 0; c < glbl_width; c++){
                            if(p2->my_shots[r][c] == 0)
                                continue;
                            int hit_at_pos = p2->my_shots[r][c] == 1;//1 if hit, -1 if miss
                            char printout[3] = {hit_at_pos ? 'H' : 'M', ' '};
                            strcat(msg_str, printout);
                            memset(helper_str, 0, sizeof(helper_str));
                            snprintf(helper_str, sizeof(helper_str), "%d %d ", c, r);
                            strcat(msg_str, helper_str);
                        }
                    }
                    
                    if(msg_str[strlen(msg_str) - 1] == ' ')
                        msg_str[strlen(msg_str) - 1] = '\0';
                    else
                        msg_str[strlen(msg_str)] = '\0';
                
                    printf("[Server] Enter message for client2: %s\n", msg_str);
                    
                    printf("strlen(msg_str): %d", strlen(msg_str));
                    
                    if(send(conn_fd_2, msg_str, strlen(msg_str), 0) < 0)
                        perror("[Server] Failed to send packet to player.");
                    memset(buffer, 0, BUFFER_SIZE);
                    read_from_c1 = 0;
                    continue;//still p2's turn so skip p1

                }
                else{
                    for(int i = 0; i < 1000; i++)
                        printf("How did we get here?");
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
                
                
                if(send(conn_fd_2, "buffer", 7, 0) < 0)
                    perror("[Server] Failed to send packet to player.");
                
            }
        }
        

        if(conn_fd_1 == -1 && conn_fd_2 == -1){
            printf("All clients disconnecting. Closing server...");
            break;
        }
    }

    for(int i = 0; i < glbl_height; i++){
        if(p1 && p1->board && p1->board[i])
            free(p1->board[i]);
        if(p1 && p1->my_shots && p1->my_shots[i])
            free(p1->my_shots[i]);
        if(p2 && p2->board && p2->board[i])
            free(p2->board[i]);
        if(p2 && p2->my_shots && p2->my_shots[i])
            free(p2->my_shots[i]);
    }
    if(p1 && p1->board)
        free(p1->board);
    if(p1 && p1->my_shots)
        free(p1->my_shots);
    if(p1)
        free(p1);
    if(p2 && p2->board)
        free(p2->board);
    if(p2 && p2->my_shots)
        free(p2->my_shots);
    if(p2)
        free(p2);
    p1 = NULL;
    p2 = NULL;


    printf("[Server] Shutting down.\n");
    close(conn_fd_1);
    close(conn_fd_2);
    close(listen_fd_1);
    close(listen_fd_2);
    return EXIT_SUCCESS;
}