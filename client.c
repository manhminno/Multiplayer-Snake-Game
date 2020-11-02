/*
 * Multiplayer Snakes game - Client
 * Luke Collins
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define PORT        7070
#define HEIGHT      24
#define WIDTH       80
#define FRUIT       -1024
#define BORDER      -99
#define REFRESH     0.15
#define WINNER      -94
#define ONGOING     -34
#define INTERRUPTED -30
#define UP_KEY      'W'
#define DOWN_KEY    'S'
#define LEFT_KEY    'A'
#define RIGHT_KEY   'D'

WINDOW* win;
char key = UP_KEY;
int game_result = ONGOING;

//Output error message and exit cleanly
void error(const char* msg){
    perror(msg);
    exit(0);
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

void* write_to_server(void* arg){
    int sockfd = *(int *) arg;
    struct timespec ts;
    ts.tv_sec = REFRESH;
    ts.tv_nsec = ((int)(REFRESH * 1000) % 1000)  * 1000000;
    while(game_result == ONGOING){
        nanosleep(&ts, NULL);
        int n = write(sockfd, &key, 1);
        if(n < 0) 
             error("ERROR writing to socket.");
     }
    return 0;
}

void* update_screen(void* arg){    
    int  sockfd = *(int*) arg;
    int  bytes_read;
    int  game_map[HEIGHT][WIDTH];
    int  map_size = HEIGHT * WIDTH * sizeof(game_map[0][0]);
    char map_buffer[map_size];
    int  i, j, n;

    while(game_result == ONGOING){

        //Recieve updated map from server
        bytes_read = 0;
        bzero(map_buffer, map_size);
        while(bytes_read < map_size){
            n = read(sockfd, map_buffer + bytes_read, map_size - bytes_read);
            if(n <= 0)
                goto end;
            bytes_read += n;
        }
        memcpy(game_map, map_buffer, map_size);

        clear();
        box(win, '+', '+');
        refresh();
        wrefresh(win);

        //for each position in the array, check if it's a snake head or bodypart
        for(i = 1; i < HEIGHT-1; i++){
            for(j = 1; j < WIDTH-1; j++){
                int current = game_map[i][j];
                int colour = abs(current) % 7;
                attron(COLOR_PAIR(colour)); 
                if((current > 0) && (current != FRUIT)){               
                    mvprintw(i, j, "  ");
                    attroff(COLOR_PAIR(colour));
                }
                else if ((current < 0) && (current != FRUIT)){
                    if(game_map[i-1][j] == -current)
                        mvprintw(i, j, "..");
                    else if(game_map[i+1][j] == -current)
                        mvprintw(i, j, "**");
                    else if(game_map[i][j-1] == -current)
                        mvprintw(i, j, " :");
                    else if(game_map[i][j+1] == -current)
                        mvprintw(i, j, ": ");
                    attroff(COLOR_PAIR(colour));
                }                
                else if (current == FRUIT){ 
                    attroff(COLOR_PAIR(colour));               
                    mvprintw(i, j, "o");                    
                }
            }
        }
        refresh();
    }

    end: game_result = game_map[0][0];
    return 0;
}

int main(int argc, char *argv[]){
    int                 sockfd;
    struct sockaddr_in  serv_addr;
    struct hostent*     server;
    char                key_buffer;

    if (argc < 2){
       fprintf(stderr,"Please type:\n\t %s [server ip]\n to launch the game.\n", argv[0]);
       exit(0);
    }    
  
    //Getting socket descriptor 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
     
    //Resolving host name
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host.\n");
        exit(0);
    }
    
    //Sets first n bytes of the area to zero    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
         
    //Converting unsigned short integer from host byte order to network byte order. 
    serv_addr.sin_port = htons(PORT);
    
    //Attempt connection with server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    //Create Ncurses Window, with input, no echo and hidden cursor
    initscr();      
    cbreak();
    noecho();
    start_color();
    use_default_colors();    
    curs_set(0);

    //Set window to new ncurses window
    win = newwin(HEIGHT, WIDTH, 0, 0);

    //Snake colours
    init_pair(0, COLOR_WHITE, COLOR_BLUE);
    init_pair(1, COLOR_WHITE, COLOR_RED);
    init_pair(2, COLOR_WHITE, COLOR_GREEN);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);
    init_pair(4, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(5, COLOR_BLACK, COLOR_CYAN);
    init_pair(6, COLOR_BLACK, COLOR_WHITE);

    mvprintw((HEIGHT-20)/2, (WIDTH-58)/2,"                          _");
    mvprintw((HEIGHT-20)/2 + 1, (WIDTH-58)/2," __                      | |");
    mvprintw((HEIGHT-20)/2 + 2, (WIDTH-58)/2,"{OO}      ___ _ __   __ _| | _____  ___");
    mvprintw((HEIGHT-20)/2 + 3, (WIDTH-58)/2,"\\__/     / __| '_ \\ / _` | |/ / _ \\/ __|");
    mvprintw((HEIGHT-20)/2 + 4, (WIDTH-58)/2," |^|     \\__ \\ | | | (_| |   <  __/\\__ \\");
    mvprintw((HEIGHT-20)/2 + 5, (WIDTH-58)/2," | |     |___/_| |_|\\__,_|_|\\_\\___||___/   v1.0  /\\");
    mvprintw((HEIGHT-20)/2 + 6, (WIDTH-58)/2," | |____________________________________________/ /");
    mvprintw((HEIGHT-20)/2 + 7, (WIDTH-58)/2," \\_______________________________________________/");
    mvprintw((HEIGHT-20)/2 + 10, (WIDTH-58)/2," Instructions:"); 
    mvprintw((HEIGHT-20)/2 + 12, (WIDTH-58)/2," - Use the keys w, a, s, d to move your snake.");
    mvprintw((HEIGHT-20)/2 + 13, (WIDTH-58)/2," - Eat fruit to grow in length.");
    mvprintw((HEIGHT-20)/2 + 14, (WIDTH-58)/2," - Do not run in to other snakes, the game border"); 
    mvprintw((HEIGHT-20)/2 + 15, (WIDTH-58)/2,"   or yourself.");
    mvprintw((HEIGHT-20)/2 + 16, (WIDTH-58)/2," - The first snake to reach length 15 wins!");
    mvprintw((HEIGHT-20)/2 + 17, (WIDTH-58)/2," - Press '.' to quit at any time.");
    mvprintw((HEIGHT-20)/2 + 19, (WIDTH-58)/2,"Press any key to start . . ."); 
    getch();

    //Start writing inputs to the server every REFRESH seconds and updating the screen
    make_thread(update_screen, &sockfd);
    make_thread(write_to_server, &sockfd);

    while(game_result == ONGOING){
        
        //Get player input with time out
        bzero(&key_buffer, 1);
        timeout(REFRESH * 1000);
        key_buffer = getch();
        key_buffer = toupper(key_buffer);
        if(key_buffer == '.'){
            game_result = INTERRUPTED;
            break;
        } else if((key_buffer == UP_KEY) 
               || (key_buffer == DOWN_KEY) 
               || (key_buffer == LEFT_KEY) 
               || (key_buffer == RIGHT_KEY))
            key = key_buffer;
    }
  
    //Show the user who won
    WINDOW* announcement = newwin(7, 35, (HEIGHT - 7)/2, (WIDTH - 35)/2);
    box(announcement, 0, 0);
    if (game_result == WINNER){
        mvwaddstr(announcement, 2, (35-21)/2, "Game Over - You WIN!");
        mvwaddstr(announcement, 4, (35-21)/2, "Press any key to quit.");
        wbkgd(announcement,COLOR_PAIR(2));
    } else{
        mvwaddstr(announcement, 2, (35-21)/2, "Game Over - you lose!");
        if(game_result > 0)
            mvwprintw(announcement, 3, (35-13)/2, "Player %d won.", game_result);
        mvwaddstr(announcement, 4, (35-21)/2, "Press any key to quit.");
        wbkgd(announcement,COLOR_PAIR(1));
    }
    mvwin(announcement, (HEIGHT - 7)/2, (WIDTH - 35)/2);
    wnoutrefresh(announcement);
    wrefresh(announcement);
    sleep(2);
    wgetch(announcement);
    delwin(announcement);
    wclear(win);
    
    echo(); 
    curs_set(1);  
    endwin();
        
    //Close connection
    close(sockfd);
    return 0;

}
