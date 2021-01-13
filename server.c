#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "process.h"

#define PORT                5500
#define MAX_PLAYERS         100
#define HEIGHT              24
#define WIDTH               80
#define MAX_SNAKE_LENGTH    HEIGHT * WIDTH
#define WINNER_LENGTH       10
#define FRUIT               -111
#define WALL                -1111
#define WALL2               -1112
#define BORDER              -99
#define WINNER              -94
#define UP_KEY              'W'
#define DOWN_KEY            'S'
#define LEFT_KEY            'A'
#define RIGHT_KEY           'D'

//Game map
int             game_map[HEIGHT+10][WIDTH+10];
int             map_size = (HEIGHT+10) * (WIDTH+10) * sizeof(game_map[0][0]);
pthread_mutex_t map_lock = PTHREAD_MUTEX_INITIALIZER;   
int             someone_won = 0;
int check_run = 0;

char *room;
int start = 0;
int t1 = 5;
int t2 = 4;
char host[256];
int number_players = 0;

//Direction key types
typedef enum{
    UP    = UP_KEY, 
    DOWN  = DOWN_KEY, 
    LEFT  = LEFT_KEY, 
    RIGHT = RIGHT_KEY
} direction;

//Coordinate structure, these are the building blocks for snakes
typedef struct{
    int x, y;
    direction d;
} coordinate;

//Snake structure, each part is made up of coordinate 
typedef struct{
    int player_no, length;
    coordinate head;
    coordinate body_segment[MAX_SNAKE_LENGTH - 2];
    coordinate tail;
} snake;

//Function to create a snake
snake* make_snake(int player_no, int head_y, int head_x){
    
    //Place the snake on the map (matrix)
    pthread_mutex_lock(&map_lock);
    game_map[head_y][head_x]   = -player_no;
    game_map[head_y+1][head_x] = 
    game_map[head_y+2][head_x] = player_no;
    pthread_mutex_unlock(&map_lock);    
    
    //Create snake struct, set coordinates facing up
    snake* s = malloc(sizeof(snake));
    
    s->player_no = player_no;
    s->length = 3;

    s->head.y = head_y;
    s->head.x = head_x;
    s->head.d = UP;

    s->body_segment[0].y = head_y + 1;
    s->body_segment[0].x = head_x;
    s->body_segment[0].d = UP;

    s->tail.y = head_y + 2;
    s->tail.x = head_x;
    s->tail.d = UP;

    return s;
}

//Function to kill snake and free memory
void kill_snake(snake* s){

    //Set all snake coordinates to zero on map
    pthread_mutex_lock(&map_lock);
    game_map[s->head.y][s->head.x] = game_map[s->tail.y][s->tail.x] = 0;    
    int i;
    for(i = 0; i < s->length - 2; i++)
        game_map[s->body_segment[i].y][s->body_segment[i].x] = 0;
    pthread_mutex_unlock(&map_lock);

    //Free memory
    free(s);    
    s = NULL;
}

//Function to move snake
void move_snake(snake* s, direction d){
    memmove(&(s->body_segment[1]), 
            &(s->body_segment[0]), 
            (s->length-2) * sizeof(coordinate));
    s->body_segment[0].y = s->head.y;
    s->body_segment[0].x = s->head.x; 
    s->body_segment[0].d = s->head.d;
    switch(d){
        case UP:{
            s->head.y = s->head.y-1;
            s->head.d = UP;            
            break;
        }
        case DOWN:{
            s->head.y = s->head.y+1;
            s->head.d = DOWN;  
            break;
        }
        case LEFT:{
            s->head.x = s->head.x-1;
            s->head.d = LEFT;  
            break;
        }
        case RIGHT:{
            s->head.x = s->head.x+1;
            s->head.d = RIGHT;  
            break;
        }
        default: break;
    }
    pthread_mutex_lock(&map_lock);
    game_map[s->head.y][s->head.x] = -(s->player_no);
    game_map[s->body_segment[0].y][s->body_segment[0].x] = s->player_no;
    game_map[s->tail.y][s->tail.x] = 0;
    pthread_mutex_unlock(&map_lock);

    s->tail.y = s->body_segment[s->length-2].y;
    s->tail.x = s->body_segment[s->length-2].x;
}

//Function to randomly add a fruit to the game map
void add_fruit(){
    int x, y;
    do{
        y = rand() % (HEIGHT - 6) + 3;
        x = rand() % (WIDTH - 6) + 3;
    } while (game_map[y][x] != 0);
    pthread_mutex_lock(&map_lock);
    game_map[y][x] = FRUIT;
    pthread_mutex_unlock(&map_lock);
}
void add_wall(){
    int x, y, a;
    do{
        y = rand() % (HEIGHT - 6) + 3;
        x = rand() % (WIDTH - 6) + 3;
    } while (game_map[y][x] != 0);
    pthread_mutex_lock(&map_lock);
    a = rand()%10;
    while(a+y >= HEIGHT || a < 3){
        a = rand()%10;
    }
    for(int i = 0; i < a; i++){
        if(game_map[y+i][x] == 0)
            game_map[y+i][x] = WALL;
    }
    pthread_mutex_unlock(&map_lock);
}

void add_wall2(){
    int x, y, a;
    do{
        y = rand() % (HEIGHT - 6) + 3;
        x = rand() % (WIDTH - 6) + 3;
    } while (game_map[y][x] != 0);
    pthread_mutex_lock(&map_lock);
    a = rand()%10;
    while(a+x >= WIDTH || a < 3){
        a = rand()%10;
    }
    for(int i = 0; i < a; i++){
        if(game_map[y][x+i] == 0)
            game_map[y][x+i] = WALL2;
    }
    pthread_mutex_unlock(&map_lock);
} 

//Function for a snake to eat a fruit in front of it
void eat_fruit(snake* s, direction d, int player_no){
    memmove(&(s->body_segment[1]), 
            &(s->body_segment[0]), 
            (s->length-2) * sizeof(coordinate));
    s->body_segment[0].y = s->head.y;
    s->body_segment[0].x = s->head.x; 
    s->body_segment[0].d = s->head.d;
    switch(d){
        case UP:{
            s->head.y = s->head.y-1;
            s->head.d = UP; 
            if(game_map[s->head.y][s->head.x + 1] == FRUIT){
                pthread_mutex_lock(&map_lock);
                game_map[s->head.y][s->head.x + 1] = 0;   
                pthread_mutex_unlock(&map_lock);        
            }
            break;
        }
        case DOWN:{
            s->head.y = s->head.y+1;
            s->head.d = DOWN; 
            if(game_map[s->head.y][s->head.x + 1] == FRUIT){
                pthread_mutex_lock(&map_lock);
                game_map[s->head.y][s->head.x + 1] = 0; 
                pthread_mutex_unlock(&map_lock);
            }
            break;
        }
        case LEFT:{
            s->head.x = s->head.x-1;
            s->head.d = LEFT;  
            break;
        }
        case RIGHT:{
            s->head.x = s->head.x+1;
            s->head.d = RIGHT;  
            break;
        }
        default: break;
    }
    pthread_mutex_lock(&map_lock);
    game_map[s->head.y][s->head.x] = -(s->player_no);
    game_map[s->body_segment[0].y][s->body_segment[0].x] = s->player_no;
    pthread_mutex_unlock(&map_lock);
    s->length++;
    game_map[HEIGHT+player_no][WIDTH+2] = s->length+10000;
    add_fruit();
}

int make_thread(void* (*fn)(void *), void* arg){
    int             err;
    pthread_t       tid;
    pthread_attr_t  attr;

    err = pthread_attr_init(&attr);
    if(err != 0)
        return err;
    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(err == 0)
        err = pthread_create(&tid, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    return err;
}

//Output error message and exit cleanly
void error(const char* msg){
    perror(msg);
    fflush(stdout);
    exit(1);
}

//Handle ctrl+c signal
void ctrl_c_handler(){
    printf("\nServer exited!.\n");
    exit(0);
}

void processRoom(char s[], char name[]){
    char tmp[256];
    const char space[2] = "_";
    char *token;
    token = strtok(s, space);
    char room_tmp[256];
    room_tmp[0] = '\0';
    strcat(room_tmp, "_");
    while(token != NULL){
        strcpy(tmp, token);
        if(strcmp(tmp, name) != 0){
            strcat(room_tmp, tmp);
        }
        token = strtok(NULL, space);
        if(token && strlen(room_tmp) != 1) strcat(room_tmp, "_");
    }
    if(room_tmp == NULL) s[0] = '\0';
    else strcpy(s, room_tmp);
}

//Thread gameplay function
void* gameplay(void* arg){ 
    if(number_players < 0) number_players = 0;
    if(strlen(room) == 0) host[0] = '\0';
    User *tmp;
    char usename[256];
    char password[256];
    List l;
    InitList(&l);
    readFile("nguoidung.txt", &l);
    int fd = *(int*) arg; 
    int player_no = fd-3;
    char recv_data[256];
    bzero(&recv_data, 12);
    int check_host = 0;
    while(1){
        int xxx = read(fd, &recv_data, 2);
        if(xxx == 0) break;
        recv_data[xxx] = '\0';
        printf("Received from client in share-socket %d: %s\n", fd, recv_data);
        if(strlen(recv_data) == 0){
            break;
        }
        if(strcmp(recv_data, "1") == 0){
            printf("Received from client in share-socket %d: Register\n", fd);
            xxx = read(fd, &usename, 256);
            if(xxx == 0){
                goto end;
            }
            usename[xxx] = '\0';
            tmp = checkUser(usename, l);
            if(tmp != NULL){
                write(fd, "NotOK", 6);
            }
            else{
                write(fd, "OK", 3);
                xxx = read(fd, &password, 256);
                if(xxx == 0){
                    goto end;
                }
                password[xxx] = '\0';
                printf("Received usename from client in share-socket %d: %s\n", fd, usename);
                printf("Received password from client in share-socket %d: %s\n", fd, password);
                int status = 0;
                int win_times = 0;
                User *p = makeUser(usename, password, status, win_times);
                addUser(&l, p);
                writeFile("nguoidung.txt", l);
            }
        }
        else if(strcmp(recv_data, "2") == 0){
            printf("Received from client in share-socket %d: Login\n", fd);
            xxx = read(fd, &usename, 256);
            if(xxx == 0){
                goto end;
            }
            usename[xxx] = '\0';
            tmp = checkUser(usename, l);
            if(tmp == NULL){
                write(fd, "NotOK", 6);
            }
            else{
                write(fd, "OK", 3);
                xxx = read(fd, &password, 256);
                if(xxx == 0){
                    break;
                }
                password[xxx] = '\0';
                printf("Receive usename from client in share-socket %d: %s\n", fd, usename);
                printf("Receive passwork from client in share-socket %d: %s\n", fd, password);
                while(strcmp(tmp->password, password) != 0){
                    write(fd, "Password sai!", 14);
                    xxx = read(fd, &password, 256);
                    if(xxx == 0){
                        break;
                    }
                    password[xxx] = '\0';
                }
                write(fd, "OKchoi", 7);
                // break;
                back:
                __fpurge(stdin);
                read(fd, &recv_data, 2);
                if(xxx == 0){
                    goto end;
                }
                recv_data[xxx] = '\0';
                if(strcmp(recv_data, "1") == 0){
                    printf("Received from client in share-socket %d: Join waitting-room\n", fd);
                    if(strlen(usename) < 1) goto end;
                    strcat(room, "_");
                    strcat(room, usename);
                    if(host[0] == '\0') strcpy(host, usename);
                    if(number_players == 9){
                        write(fd, "maxplayers", 11);
                        goto end;
                    }
                    if(check_run == 1){
                        write(fd, "running", 8);
                        room[0] = '\0';
                        goto back;
                    }
                    number_players += 1;
                    write(fd, room, 256);
                    printf("||Number of players are accessing: %d||\n", number_players);
                    while(1){
                        xxx = read(fd, &recv_data, 256);
                        if(xxx == 0){
                            // goto end;
                            break;
                        }
                        if(strcmp(recv_data, "S") == 0 || strcmp(recv_data, "s") == 0){
                            strcpy(recv_data, "start");
                            write(fd, recv_data, 256);
                            check_run = 1;
                            start = 1;
                            sleep(t2);
                            check_host = 1;
                            break;
                        }
                        else if(strcmp(recv_data, "Q") == 0 || strcmp(recv_data, "q") == 0){
                            strcpy(recv_data, "quit");
                            write(fd, recv_data, 256);
                            processRoom(room, tmp->usename);
                            if(strlen(room) == 1) room = '\0';
                            // number_players -= 1;
                            // printf("ssssss\n");
                            goto end;
                            break;
                        }
                        if(start == 1){
                            t1 = 3;
                            strcpy(recv_data, "start");
                            write(fd, recv_data, 256);
                            break;
                        }
                        else{
                            write(fd, room, 256);
                        }
                        // start = 1;
                        sleep(t1);
                    }
                    break;
                }
                else if(strcmp(recv_data, "2") == 0){
                    printf("Received from client in share-socket %d: Change password\n", fd);
                    char new[256];
                    xxx = read(fd, &new, 256);
                    if(xxx == 0) break;
                    // printf("nhan: %s\n", new);
                    strcpy(tmp->password, new);
                    writeFile("nguoidung.txt", l);
                    goto back;
                }
                else if(strcmp(recv_data, "3") == 0){
                    printf("Received from client in share-socket %d: Show profile\n", fd);
                    char information[256];
                    strcat(information, "_");
                    strcat(information, tmp->usename);
                    strcat(information, "_");
                    char c[3]; 
                    if(tmp->status > 9){
                        c[0] = tmp->status/10 + '0';
                        c[1] = tmp->status - 10*(tmp->status/10) + '0';
                        c[2] = '\0';
                    }
                    else{
                        c[0] = tmp->status + '0';
                        c[1] = '\0';
                    }
                    strcat(information, c);
                    strcat(information, "_");
                    if(tmp->win_times > 9){
                        c[0] = tmp->win_times/10 + '0';
                        c[1] = tmp->win_times - 10*(tmp->win_times/10) + '0';
                        c[2] = '\0';
                    }
                    else{
                        c[0] = tmp->win_times + '0';
                        c[1] = '\0';
                    }
                    strcat(information, c);
                    write(fd, information, 256);
                    xxx = read(fd, &recv_data, 256);
                    if(xxx == 0) break;
                    if(strcmp(recv_data, "3") == 0){
                        information[0] = '\0';
                        goto back;
                    }
                }
                else if(strcmp(recv_data, "4") == 0){
                    printf("Received from client in share-socket %d: Show leaderboard\n", fd);
                    xxx = read(fd, &recv_data, 256);
                    if(xxx == 0) break;
                    if(strcmp(recv_data, "1") == 0){
                        sortListStatus(&l);
                        User *user_tmp = l.pHead;
                        char information[256];
                        for(int i = 0; i < 9; i++){
                            strcat(information, "_");
                            strcat(information, user_tmp->usename);
                            strcat(information, "_");
                            char c[3]; 
                            if(user_tmp->status > 9){
                                c[0] = user_tmp->status/10 + '0';
                                c[1] = user_tmp->status - 10*(user_tmp->status/10) + '0';
                                c[2] = '\0';
                            }
                            else{
                                c[0] = user_tmp->status + '0';
                                c[1] = '\0';
                            }
                            strcat(information, c);
                            strcat(information, "_");
                            if(user_tmp->win_times > 9){
                                c[0] = user_tmp->win_times/10 + '0';
                                c[1] = user_tmp->win_times - 10*(user_tmp->win_times/10) + '0';
                                c[2] = '\0';
                            }
                            else{
                                c[0] = user_tmp->win_times + '0';
                                c[1] = '\0';
                            }
                            strcat(information, c);
                            write(fd, information, 256);
                            // printf("%s\n", information);
                            user_tmp = user_tmp->pNext;
                            information[0] = '\0';
                        }
                        xxx = read(fd, &recv_data, 256);
                        if(xxx == 0) break;
                        if(strcmp(recv_data, "4") == 0){
                            goto back;
                        }
                    }
                    else if(strcmp(recv_data, "2") == 0){
                        sortListWontimes(&l);
                        User *user_tmp = l.pHead;
                        char information[256];
                        for(int i = 0; i < 9; i++){
                            strcat(information, "_");
                            strcat(information, user_tmp->usename);
                            strcat(information, "_");
                            char c[3]; 
                            if(user_tmp->status > 9){
                                c[0] = user_tmp->status/10 + '0';
                                c[1] = user_tmp->status - 10*(user_tmp->status/10) + '0';
                                c[2] = '\0';
                            }
                            else{
                                c[0] = user_tmp->status + '0';
                                c[1] = '\0';
                            }
                            strcat(information, c);
                            strcat(information, "_");
                            if(user_tmp->win_times > 9){
                                c[0] = user_tmp->win_times/10 + '0';
                                c[1] = user_tmp->win_times - 10*(user_tmp->win_times/10) + '0';
                                c[2] = '\0';
                            }
                            else{
                                c[0] = user_tmp->win_times + '0';
                                c[1] = '\0';
                            }
                            strcat(information, c);
                            write(fd, information, 256);
                            // printf("%s\n", information);
                            user_tmp = user_tmp->pNext;
                            information[0] = '\0';
                        }
                        xxx = read(fd, &recv_data, 256);
                        if(xxx == 0) break;
                        if(strcmp(recv_data, "4") == 0){
                            goto back;
                        }
                    }
                }
                else if(strcmp(recv_data, "5") == 0){
                    printf("Received from client in share-socket %d: Quit game\n", fd);
                    if(strcmp(recv_data, "5") == 0){
                        goto end;
                    }
                }
            }
        }
        // break;
    }
    printf("Player %d had connected to game!\n", player_no);
    if(tmp != NULL){
        tmp->status += 1;
        writeFile("nguoidung.txt", l);
    }
    // printf("%s\n", host);
    if(start == 1){
        room[0] = '\0';
        number_players = 0;
    }
    if(check_host == 0 && strcmp(tmp->usename, host) == 0){
        // start = 1;
        // room[0] = '\0';
        host[0] = '\0';
        // check_host = 0;
        processRoom(room, tmp->usename);
        // printf("%s---\n", room);
        if(strlen(room) == 1) room[0] = '\0';
        number_players -= 1;
    }
    else{
        processRoom(room, tmp->usename);
        start = 0;
        check_host = 0;
    }

    //Determine player number from file descriptor argument
    end:
    if(number_players != 0) number_players -= 1;
    printf("||Number of players are accessing: %d||\n", number_players);

    //Find three consecutive zeros in map for starting snake position
    int head_y, head_x;
    srand(time(NULL));
    do{
        head_y = rand() % (HEIGHT - 6) + 3;
        head_x = rand() % (WIDTH - 6) + 3;
    } while (!(((game_map[head_y][head_x] == game_map[head_y+1][head_x]) == game_map[head_y+2][head_x]) == 0));

    //Create snake structure
    snake* player_snake = make_snake(player_no, head_y, head_x);

    //Variables for user input
    char key = UP;
    char key_buffer;
    char map_buffer[map_size];
    int  bytes_sent, n;
    int  success = 1;

    while(success){
        //Check if someone won
        if(someone_won)
            success = 0;

        //Check if you are the winner
        if(player_snake->length-3 >= WINNER_LENGTH){
            someone_won = player_no;
            pthread_mutex_lock(&map_lock);
            game_map[0][0] = WINNER;
            pthread_mutex_unlock(&map_lock);
        } else if(game_map[0][0] != BORDER){
            pthread_mutex_lock(&map_lock);
            game_map[0][0] = someone_won;
            pthread_mutex_unlock(&map_lock);
        }

        //Copy map to buffer, and send to client
        memcpy(map_buffer, game_map, map_size);
        bytes_sent = 0;
        while(bytes_sent < map_size){         
            bytes_sent += write(fd, game_map, map_size);
            if (bytes_sent < 0) error("ERROR writing to socket");
        } 

        //Player key input
        bzero(&key_buffer, 1);
        n = read(fd, &key_buffer, 1);
        if (n <= 0)
            break;

        //If user key is a direction, then apply it
        key_buffer = toupper(key_buffer);   
        if(  ((key_buffer == UP)    && !(player_snake->head.d == DOWN))
        ||((key_buffer == DOWN)  && !(player_snake->head.d == UP))
        ||((key_buffer == LEFT)  && !(player_snake->head.d == RIGHT)) 
        ||((key_buffer == RIGHT) && !(player_snake->head.d == LEFT)))
            key = key_buffer;

        switch(key){

            case UP:{
                if((game_map[player_snake->head.y-1][player_snake->head.x] == 0) && 
                    !(game_map[player_snake->head.y-1][player_snake->head.x+1] == FRUIT)){
                    move_snake(player_snake, UP);
                }
                else if((game_map[player_snake->head.y-1][player_snake->head.x] == FRUIT) || 
                    (game_map[player_snake->head.y-1][player_snake->head.x+1] == FRUIT)){
                    eat_fruit(player_snake, UP, player_no);
                }
                else{
                    move_snake(player_snake, LEFT);
                    success = 0;
                }
                break;
            }

            case DOWN:{
                if((game_map[player_snake->head.y+1][player_snake->head.x] == 0)&& 
                    !(game_map[player_snake->head.y+1][player_snake->head.x+1] == FRUIT)){
                    move_snake(player_snake, DOWN);
                }
                else if((game_map[player_snake->head.y+1][player_snake->head.x] == FRUIT) || 
                    (game_map[player_snake->head.y+1][player_snake->head.x+1] == FRUIT)){
                    eat_fruit(player_snake, DOWN, player_no);
                }
                else{
                    move_snake(player_snake, DOWN);
                    success = 0;
                }
                break;
            }

            case LEFT:{
                if(game_map[player_snake->head.y][player_snake->head.x-1] == 0){
                    move_snake(player_snake, LEFT);
                }
                else if(game_map[player_snake->head.y][player_snake->head.x-1] == FRUIT){
                    eat_fruit(player_snake, LEFT, player_no);

                }
                else{
                    move_snake(player_snake, LEFT);
                    success = 0;
                }
                break;
            }

            case RIGHT:{
                if(game_map[player_snake->head.y][player_snake->head.x+1] == 0){
                    move_snake(player_snake, RIGHT);
                }
                else if(game_map[player_snake->head.y][player_snake->head.x+1] == FRUIT){
                    eat_fruit(player_snake, RIGHT, player_no);
                }
                else{
                    move_snake(player_snake, RIGHT);
                    success = 0;
                }
                break;
            }

            default: break;
        }   
    }
    if(player_snake->length-3 == WINNER_LENGTH){
        fprintf(stderr, "Player %d had won!\n", player_no);
        tmp->win_times += 1;
        writeFile("nguoidung.txt", l);
        kill_snake(player_snake);
        game_map[HEIGHT+player_no][WIDTH+2] = 0;
        close(fd); 
        number_players = 0;
        check_run = 0; 
        host[0] = '\0';
        return 0;
    }
    else{
        fprintf(stderr, "Player %d had exited game!\n", player_no);
        kill_snake(player_snake);
        game_map[HEIGHT+player_no][WIDTH+2] = 0;
        if(number_players == 0) check_run = 0;
        close(fd);  
        return 0;
    }
}

//Main function
int main(){
    room = (char *)malloc(256*sizeof(char));
    int                socket_fds[MAX_PLAYERS];     
    struct sockaddr_in socket_addr[MAX_PLAYERS];
    int                i;

    //Handle Ctrl+C
    signal(SIGINT, ctrl_c_handler);

    //Fill gamestate matrix with zeros
    memset(game_map, 0, map_size);
    
    //Set game borders
    for(i = 0; i < HEIGHT; i++)
        game_map[i][0] = game_map[i][WIDTH-2] = BORDER;     
    for(i = 0; i < WIDTH; i++)
        game_map[0][i] = game_map[HEIGHT-1][i] = BORDER;

    //Randomly add five fruit
    srand(time(NULL));
    for(i = 0; i < 5; i++)
        add_fruit();
    for(i = 0; i < 3; i++)
        add_wall();
    for(i = 0; i < 2; i++)
        add_wall2();
    //Create server socket
    socket_fds[0] = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fds[0] < 0) 
        error("ERROR opening socket");
        
    //Set socket address to zero and set attributes
    bzero((char *) &socket_addr[0], sizeof(socket_addr[0]));  
    socket_addr[0].sin_family = AF_INET;
    socket_addr[0].sin_addr.s_addr = INADDR_ANY;
    //Converting unsigned short integer from host byte order to network byte order. 
    socket_addr[0].sin_port = htons(PORT);
    
    //Assigning address specified by addr to the socket referred by the server socket fd
    if (bind(socket_fds[0], (struct sockaddr *) &socket_addr[0], sizeof(socket_addr[0])) < 0) 
            error("ERROR on binding");

    //Marking socket as a socket that will be used to accept incoming connection requests  
    listen(socket_fds[0], 9);
    socklen_t clilen = sizeof(socket_addr[0]);

    while(1){
        for(i = 1;; i++){
            //Accepting an incoming connection request
            socket_fds[i] = accept(socket_fds[0], (struct sockaddr *) &socket_addr[i], &clilen);
            if (socket_fds[i] < 0) 
                error("ERROR on accept");
            
            if(someone_won){
                someone_won = 0;
            }
            make_thread(&gameplay, &socket_fds[i]); 
        }
        //Closing the server socket
        close(socket_fds[0]);  
    }
    return 0; 
}
