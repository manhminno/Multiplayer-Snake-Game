#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio_ext.h>

#define MAX 255
#define BUFF_SIZE 255

int login_value = 0;
char login_user[MAX];

typedef struct _User{
    char usename[MAX];
    char password[MAX];
    int status;
    int win_times;
    struct _User *pNext;
} User;

typedef struct List{
    User *pHead;
    User *pTail;
} List;

void InitList(List *l){
    l->pHead = l->pTail = NULL;
}

User *makeUser(char usename[], char password[], int status, int win_times){
    User *p = (User *)malloc(sizeof(User));
    strcpy(p->usename, usename);
    strcpy(p->password, password);
    p->status = status;
    p->win_times = win_times;
    p->pNext = NULL;
    return p;
}

void addUser(List *l, User *p){
    if(l->pHead == NULL)
        l->pHead = l->pTail = p;
    else{
        l->pTail->pNext = p;
        l->pTail = p;
    }
}

int checkBye(char needCheck[]){
    if(strcmp(needCheck, "Bye") == 0 || strcmp(needCheck, "bye") == 0 || strcmp(needCheck, "BYE") == 0) return 1;
    return 0;
}

void readFile(char *file_name, List *l){
    FILE *fin = fopen(file_name, "r");

    if (fin == NULL) {
        printf("Can't open file\n");
        return;
    }
    char *token;
    char tmp[MAX];
    char tmp2[MAX];
    char usename[MAX];
    char password[MAX];
    int status;
    int win_times;
    const char space[2] = " ";

    while(!feof(fin)){
        fgets(tmp2, 256, fin);
        token = strtok(tmp2, space);
        strcpy(usename, token);
        token = strtok(NULL, space);
        strcpy(password, token);
        token = strtok(NULL, space);
        strcpy(tmp, token);
        if(strlen(tmp) > 1){
            status = (tmp[0] - '0')*10 + (tmp[1] - '0');
        }
        else status = tmp[0] - '0';
        token = strtok(NULL, space);
        strcpy(tmp, token);
        // printf("%s---\n",tmp);
        if(tmp[strlen(tmp)-1] == '\n')  tmp[strlen(tmp)-1] = '\0';
        if(strlen(tmp) > 1){
            win_times = (tmp[0] - '0')*10 + (tmp[1] - '0');
        }
        else win_times = tmp[0] - '0';
        User *p = makeUser(usename, password, status, win_times);
        addUser(l, p);
    }
    fclose(fin);
}

User *checkUser(char needCheck[], List l){
    for(User *p = l.pHead; p != NULL; p = p->pNext){
        if(strcmp(p->usename, needCheck) == 0) return p;
    }
    return NULL;
}

void writeFile(char *file_name, List l){
    FILE *fin = fopen(file_name, "w");
    for(User *p = l.pHead; p != NULL; p = p->pNext){
        fprintf(fin,"%s", p->usename);
        fprintf(fin, "%s", " ");
        fprintf(fin, "%s", p->password);
        fprintf(fin, "%s", " ");
        char c[3]; 
        if(p->status > 9){
            c[0] = p->status/10 + '0';
            c[1] = (p->status - 10*(p->status/10)) + '0';
            c[2] = '\0';
        }
        else{
            c[0] = p->status + '0';
            c[1] = '\0';
        }
        fprintf(fin, "%s", c);
        fprintf(fin, "%s", " ");
        if(p->win_times > 9){
            c[0] = p->win_times/10 + '0';
            c[1] = p->win_times - 10*(p->win_times/10) + '0';
            c[2] = '\0';
        }
        else{
            c[0] = p->win_times + '0';
            c[1] = '\0';
        }
        fprintf(fin, "%s", c);
        if(p != l.pTail) fprintf(fin, "\n");
    }
    fclose(fin);
}

int checkBuff(char needCheck[], char *number, char *alpha){
	int countNumber = 0, countAlphabet = 0;

	for (int i = 0; i < strlen(needCheck); i++)
	{
		if(needCheck[i] > 47 && needCheck[i] < 58){
			number[countNumber] = needCheck[i];
			countNumber ++;
		}
        else if((needCheck[i] > 64 && needCheck[i] < 91) || (needCheck[i] > 96 && needCheck[i] < 123)){
            alpha[countAlphabet] = needCheck[i];
            countAlphabet ++;
        }
        else return 0;
	}
    return 1;
}

void sendtoClient(char tmp_send[], int sockfd){
    send(sockfd, tmp_send, strlen(tmp_send), 0);
}

int loginUser_server(char *file_name, List *l, int sockfd){
    char usename[MAX];
    char password[MAX];
    int check = 1;
    char tmp_send[MAX];
    char tmp_recv[MAX];
    char *number = (char *)malloc(MAX*sizeof(char));
    char *alpha = (char *)malloc(MAX*sizeof(char));
    char *tmp2 = (char *)malloc(MAX*sizeof(tmp2));
    int newcheck;

    int rcvBytes;

    //Send some print
    sendtoClient("Login User:\nUsername: ", sockfd);

    //Get usename
    rcvBytes = recv(sockfd, tmp_recv, MAX-1, 0);
    tmp_recv[rcvBytes] = '\0';
    if(checkBye(tmp_recv)) return 0;
    printf("Client send usename: %s\n", tmp_recv);
    strcpy(usename, tmp_recv);

    //Check active or non or NULL
    User *tmp = checkUser(usename, *l);
    if(tmp != NULL){
        if(tmp->status == 0){
            sendtoClient("Account is blocked\n", sockfd);
            return 1;
        }
        else if(tmp->status == 2){
            sendtoClient("Account is not activated. Activation required.\n", sockfd);
            return 1;
        }

        //Get pass
        sendtoClient("Password: ", sockfd);
        rcvBytes = recv(sockfd, tmp_recv, MAX-1, 0);
        tmp_recv[rcvBytes] = '\0';
        if(checkBye(tmp_recv)) return 0;

        printf("Client send pass: %s\n", tmp_recv);
        strcpy(password, tmp_recv);

        if(strcmp(tmp->password, password) == 0){
            sendtoClient("OK\n", sockfd);

            //Change pass
            rcvBytes = recv(sockfd, tmp_recv, MAX-1, 0);
            tmp_recv[rcvBytes] = '\0';
            if(checkBye(tmp_recv)) return 0;

            newcheck = checkBuff(tmp_recv, number, alpha);
            printf("Client send new pass: %s\n", tmp_recv);
            
            if(newcheck){
                __fpurge(stdin);
                strcpy(tmp2,"Change pass successful!\nNewpass: ");
                if(strlen(number) != 0) tmp2 = strcat(tmp2, number);
                if(strlen(alpha) != 0) tmp2 = strcat(tmp2, alpha);
                sendtoClient(tmp2, sockfd);
                strcpy(tmp->password, tmp_recv);
                writeFile("nguoidung.txt", *l);
            }else{
                sendtoClient("Error! Wrong pass!\n", sockfd);
            }
            free(number);
            free(alpha);
            free(tmp2);
            return 1;
        }
        else{
            while(check != 3){
                sendtoClient("Password is incorrect\nPassword: ", sockfd);

                rcvBytes = recv(sockfd, tmp_recv, MAX-1, 0);
                tmp_recv[rcvBytes] = '\0';
                if(checkBye(tmp_recv)) return 0;
        
                printf("Client send pass: %s\n", tmp_recv);
                strcpy(password, tmp_recv);

                if(strcmp(tmp->password, password) == 0){
                    sendtoClient("OK\n", sockfd);

                    //Change passs
                    rcvBytes = recv(sockfd, tmp_recv, MAX-1, 0);
                    tmp_recv[rcvBytes] = '\0';
                    if(checkBye(tmp_recv)) return 0;
                    newcheck = checkBuff(tmp_recv, number, alpha);

                    printf("Client send new pass: %s\n", tmp_recv);
                    if(newcheck){
                        __fpurge(stdin);
                        strcpy(tmp2,"Change pass successful!\nNewpass: ");
                        if(strlen(number) != 0) tmp2 = strcat(tmp2, number);
                        if(strlen(alpha) != 0) tmp2 = strcat(tmp2, alpha);
                        strcpy(tmp_send, tmp2);
                        sendtoClient(tmp_send, sockfd);
                        strcpy(tmp->password, tmp_recv);
                        writeFile("nguoidung.txt", *l);
                    }else{
                        sendtoClient("Error! Wrong pass!\n", sockfd);
                    }
                    free(number);
                    free(alpha);
                    free(tmp2);
                    return 1;
                }
                check += 1;
            }
            tmp->status = 0;
            writeFile("nguoidung.txt", *l);
            sendtoClient("Password is incorrect. Account is blocked\n", sockfd);
            return 1;
        }
    }
    else{
        sendtoClient("Cannot find account!\n", sockfd);
        return 1;
    }
    return 1;
}