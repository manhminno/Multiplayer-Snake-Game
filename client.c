#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <ncurses.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdio_ext.h>
#include "process.h"

#define BUFF_SIZE 255
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

void Snake(){
    system("clear");
    printf("                          _\n");
    printf(" __                      | |\n");
    printf("{OO}      ___ _ __   __ _| | _____  ___\n");
    printf("\\__/     / __| '_ \\ / _` | |/ / _ \\/ __|\n");
    printf(" |^|     \\__ \\ | | | (_| |   <  __/\\__ \\\n");
    printf(" | |     |___/_| |_|\\__,_|_|\\_\\___||___/   HUST  /\\\n");
    printf(" | |____________________________________________/ /\n");
    printf(" \\_______________________________________________/\n");
}

void Menu(){
    // Snake();
    printf(" _________________________________________________ \n");
    printf("|                 => [1]. Register                |\n");
    printf("|                 => [2]. Login                   |\n");
    printf("|_________________________________________________|\n");
    printf("Play now: ");
}

char *showRoom(char room[]){
    Snake();
    printf(" __________________Waiting-room___________________ \n");
    const char space[2] = "_";
    char *token;
    char tmp[256];
    int i = 1;
    char *main_player;
    token = strtok(room, space);
    main_player = token;
    strcpy(tmp, token);
    printf(">[No %d]. %s\n", i, tmp);
    i++;
    token = strtok(NULL, space);
    while(token != NULL){
        strcpy(tmp, token);
        printf(">[No %d]. %s\n", i, tmp);
        i++;
        token = strtok(NULL, space);
    }
    printf(" _________________________________________________ \n");
    printf(" Waiting-room will be updated every 5 seconds!\n");
    return main_player;
}

void showProfile(char user[]){
    Snake();
    printf(" _____________________Profile_____________________ \n");
    const char space[2] = "_";
    char *token;
    char tmp[256];
    float playedtimes;
    float wontimes;
    float pw;
    token = strtok(user, space);
    strcpy(tmp, token);
    printf(">>Account     : %s\n", tmp);
    token = strtok(NULL, space);
    strcpy(tmp, token);
    if(strlen(tmp) > 1){
        playedtimes = (tmp[0] - '0')*10 + (tmp[1] - '0');
    }
    else playedtimes = tmp[0] - '0';
    printf(">>Played-times: %s\n", tmp);
    token = strtok(NULL, space);
    strcpy(tmp, token);
    if(strlen(tmp) > 1){
        wontimes = (tmp[0] - '0')*10 + (tmp[1] - '0');
    }
    else wontimes = tmp[0] - '0';
    printf(">>Won-times   : %s\n", tmp);
    if(wontimes == 0) pw = 0;
    else pw = playedtimes/wontimes;
    printf(">>PW-rate     : %.2f\n", pw);
}

void sign_to_server(int sockfd){
    char test[256];
    char choice[2];
    char usename[256];
    char password[256];
    char tmp[256];
    int signup = 0;
    // List l;
    // InitList(&l);
    // User *p;
    while(1){

        Snake();
        if(signup == 1){
            printf("|             **Sign up successful!**             |\n");
        }
        else if(signup == -2){
            printf("|       **Error! Username already exists!**       |\n");
        }
        Menu();
        __fpurge(stdin);
        gets(choice);
        // test = choice - '0';
        // printf("%s\n", test);
        int check = choice[0] - '0';
        write(sockfd, choice, 2);
        switch (check){
            case 1:
                Snake();
                printf(" ________________Register account__________________\n");
                printf("Username: ");
                __fpurge(stdin);
                gets(usename);
                write(sockfd, usename, 256);
                read(sockfd, &test, 10);
                if(strcmp(test, "NotOK") == 0){
                    // printf("Error! Username already exists!\n");
                    signup = -2;
                    break;
                }else{
                    printf("Password: ");
                    __fpurge(stdin);
                    gets(password);
                    while(strlen(password) < 6){
                        printf("Password length must be greater than or equal to 6 characters!\n");
                        printf("Password: ");
                        __fpurge(stdin);
                        gets(password);
                    }
                    write(sockfd, password, 256);
                }
                signup = 1;
                break;
            case 2:
                Snake();
                printf(" _________________Login account___________________\n");
                printf("Username: ");
                __fpurge(stdin);
                gets(usename);
                write(sockfd, usename, 256);
                read(sockfd, &test, 10);
                if(strcmp(test, "NotOK") == 0){
                    printf("Error! Wrong username!\n");
                    break;
                }else{
                    strcpy(tmp, usename);
                    printf("Password: ");
                    __fpurge(stdin);
                    gets(password);
                    write(sockfd, password, 256);
                    read(sockfd, &test, 256);
                    while(strcmp(test, "OKchoi") != 0){
                        printf("Error! Wrong password!\n");
                        printf("Password: ");
                        __fpurge(stdin);
                        gets(password);
                        write(sockfd, password, 256);
                        read(sockfd, &test, 256);
                    }
                    back:
                    Snake();
                    if(signup == -3){
                        printf("|       **Password changed successfully!**        |\n");
                        printf("|_________________________________________________|\n");
                    }
                    else{
                        printf(" ____________Logged in successfully!______________ \n");
                    }
                    printf("|             => [1]. Join waiting-room           |\n");
                    printf("|             => [2]. Change password             |\n");
                    printf("|             => [3]. Show profile                |\n");
                    printf("|             => [4]. Show leaderboard            |\n");
                    printf("|_________________________________________________|\n");
                    printf("===> ");
                    __fpurge(stdin);
                    gets(choice);
                    while(strlen(choice) == 0){
                        printf("===> ");
                        __fpurge(stdin);
                        gets(choice);
                    }
                    // test = choice - '0';
                    // printf("%s\n", test);
                    int check2 = choice[0] - '0';
                    char new_password[256];
                    switch(check2){
                        case 1:
                            write(sockfd, choice, 2);
                            read(sockfd, &test, 256);
                            while(1){
                                // printf("%s", test);
                                // printf("useeeeeerr: %s", usename);
                                char *test2 = showRoom(test);
                                // char c;
                                if(strcmp(usename, test2) == 0){
                                    // free(test2);
                                    printf("\n You are host of the room, let's start game!\n");
                                    printf("        =>[S]. Start game\n");
                                    printf("        =>[T]. Waiting for more players\n");
                                    printf("=>");
                                    gets(test);
                                    write(sockfd, test, 256);
                                }else{
                                    strcpy(test, "accc");
                                    write(sockfd, test, 256);
                                }
                                read(sockfd, &test, 256);
                                if(strcmp(test, "start") == 0){
                                    // free(test2);
                                    return;
                                }
                            }
                        case 2:
                            write(sockfd, choice, 2);
                            printf("New password: ");
                            gets(new_password);
                            while(strlen(new_password) < 6){
                                printf("Password length must be greater than or equal to 6 characters!\n");
                                printf("New password: ");
                                gets(new_password);
                            }
                            // printf("%s\n", new_password);
                            write(sockfd, new_password, 256);
                            signup = -3;
                            goto back;
                            break;
                        case 3:
                            signup = 0;
                            write(sockfd, choice, 2);
                            read(sockfd, &test, 256);
                            // printf("%s\n", test);
                            showProfile(test);
                            printf("Press enter to continue...");
                            getchar();
                            write(sockfd, choice, 2);                      
                            goto back;
                        case 4:
                            signup = 0;
                            write(sockfd, choice, 2);
                            Snake();
                            printf(" ______________________Leaderboard______________________ \n");
                            printf("| Top |    Account    | Played-times |Won-times| PW-rate|\n");
                            for(int i = 0; i < 9; i++){
                                read(sockfd, &test, 256);
                                char *token;
                                char tmp[256];
                                float playedtimes;
                                float wontimes;
                                float pw;
                                const char space[2] = "_";
                                if(i<3){
                                    if(i == 0) printf("***%d***\t", i+1);
                                    else if(i == 1) printf(" **%d**\t", i+1);
                                    else if(i == 2) printf("  *%d*\t", i+1);
                                }
                                else printf("  [%d]\t", i+1);
                                token = strtok(test, space);
                                strcpy(tmp, token);
                                printf("%-22s", tmp);
                                token = strtok(NULL, space);
                                strcpy(tmp, token);
                                if(strlen(tmp) > 1){
                                    playedtimes = (tmp[0] - '0')*10 + (tmp[1] - '0');
                                }
                                else playedtimes = tmp[0] - '0';
                                printf("%-11s", tmp);
                                token = strtok(NULL, space);
                                strcpy(tmp, token);
                                if(strlen(tmp) > 1){
                                    wontimes = (tmp[0] - '0')*10 + (tmp[1] - '0');
                                }
                                else wontimes = tmp[0] - '0';
                                printf("%-9s", tmp);
                                if(wontimes == 0) pw = 0;
                                else pw = playedtimes/wontimes;
                                printf("%.2f\n", pw);
                                // printf("%s\n", test);
                            }
                            printf("Press enter to continue...");
                            getchar();
                            write(sockfd, choice, 2);                      
                            goto back;
                        default:
                            break;
                    }
                    
                }
                break;

            default:
                break;
        }
    }
    return;
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
        box(win, 0, 0);
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
        fprintf(stderr,"Please type: %s [server ip] to launch the game.\n", argv[0]);
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
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        
    //Converting unsigned short integer from host byte order to network byte order. 
    serv_addr.sin_port = htons(PORT);
    
    //Attempt connection with server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    sign_to_server(sockfd);


    printf("Tro choi se bat dau sau: 5\n");
    sleep(1);
    printf("Tro choi se bat dau sau: 4\n");
    sleep(1);
    printf("Tro choi se bat dau sau: 3\n");
    sleep(1);
    printf("Tro choi se bat dau sau: 2\n");
    sleep(1);
    printf("Tro choi se bat dau sau: 1\n");
    sleep(1);


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
        } else if((key_buffer == UP_KEY) || (key_buffer == DOWN_KEY) || (key_buffer == LEFT_KEY) || (key_buffer == RIGHT_KEY))
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
