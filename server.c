/*
 * Multiplayer Snakes game - Server
 * Luke Collins
 */
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

#define PORT                7070
#define MAX_PLAYERS         1024
#define HEIGHT              24
#define WIDTH               80
#define MAX_SNAKE_LENGTH    HEIGHT * WIDTH
#define WINNER_LENGTH       15
#define FRUIT               -1024
#define BORDER              -99
#define WINNER              -94
#define UP_KEY              'W'
#define DOWN_KEY            'S'
#define LEFT_KEY            'A'
#define RIGHT_KEY           'D'

//Game map
int             game_map[HEIGHT][WIDTH];
int             map_size = HEIGHT * WIDTH * sizeof(game_map[0][0]);
pthread_mutex_t map_lock = PTHREAD_MUTEX_INITIALIZER;   
int             someone_won = 0;

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
    game_map[s->head.y][s->head.x] = 
    game_map[s->tail.y][s->tail.x] = 0;    
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

//Function for a snake to eat a fruit in front of it
void eat_fruit(snake* s, direction d){
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
    add_fruit();
}

//Stevens, chapter 12, page 428: Create detatched thread
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
    printf("\nIs-server ġie maqtul.\n");
    exit(0);
}

//Thread gameplay function
void* gameplay(void* arg){  

    //Determine player number from file descriptor argument
    int fd = *(int*) arg;
    int player_no = fd-3;
    printf("Dahal plejer numru %d!\n", player_no);

    //Find three consecutive zeros in map for starting snake position
    int head_y, head_x;
    srand(time(NULL));
    do{
        head_y = rand() % (HEIGHT - 6) + 3;
        head_x = rand() % (WIDTH - 6) + 3;
    } while (!(
        ((game_map[head_y][head_x] == game_map[head_y+1][head_x]) 
            == game_map[head_y+2][head_x]) == 0));

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
        if(player_snake->length >= 15){
            someone_won = player_no;
            pthread_mutex_lock(&map_lock);
            game_map[0][0] = WINNER;
            pthread_mutex_unlock(&map_lock);
        } else if(game_map[0][0]!= BORDER){
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
                    printf("Plejer %d mexa 'l fuq!\n",player_no);
                }
                else if((game_map[player_snake->head.y-1][player_snake->head.x] == FRUIT) || 
                    (game_map[player_snake->head.y-1][player_snake->head.x+1] == FRUIT)){
                    eat_fruit(player_snake, UP);
                    printf("Plejer %d kiel frotta!\n",player_no);
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
                    printf("Plejer %d mexa 'l isfel!\n",player_no);
                }
                else if((game_map[player_snake->head.y+1][player_snake->head.x] == FRUIT) || 
                    (game_map[player_snake->head.y+1][player_snake->head.x+1] == FRUIT)){
                    eat_fruit(player_snake, DOWN);
                    printf("Plejer %d kiel frotta!\n",player_no);
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
                    printf("Plejer %d mexa lejn ix-xellug!\n",player_no);
                }
                else if(game_map[player_snake->head.y][player_snake->head.x-1] == FRUIT){
                    eat_fruit(player_snake, LEFT);
                    printf("Plejer %d kiel frotta!\n",player_no);
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
                    printf("Plejer %d mexa lejn il-lemin!\n",player_no);
                }
                else if(game_map[player_snake->head.y][player_snake->head.x+1] == FRUIT){
                    eat_fruit(player_snake, RIGHT);
                    printf("Plejer %d kiel frotta!\n",player_no);
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

    if(player_snake->length == WINNER_LENGTH){
        fprintf(stderr, "Plejer %d rebaħ!\n", player_no);
        kill_snake(player_snake);
        close(fd);  
        return 0;
    }

    else{
        fprintf(stderr, "Plejer %d telaq mil-logħba.\n", player_no);
        kill_snake(player_snake);
        close(fd);  
        return 0;
    }
}

//Main function
int main(){
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
    for(i = 0; i < 3; i++)
        add_fruit();

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
    listen(socket_fds[0], 5);
    socklen_t clilen = sizeof(socket_addr[0]);

     for(i = 1;; i++){
        
         //Accepting an incoming connection request
         socket_fds[i] = accept(socket_fds[0], (struct sockaddr *) &socket_addr[i], &clilen);
         if (socket_fds[i] < 0) 
              error("ERROR on accept");

         //Reset game if someone won
         if(someone_won){
            printf("Il-logħba ġiet irrisettjata!\n");
            someone_won = 0;
        }

         make_thread(&gameplay, &socket_fds[i]); 
     }
     
     //Closing the server socket
     close(socket_fds[0]);  
     return 0; 
}
