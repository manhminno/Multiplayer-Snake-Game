#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio_ext.h>

#define MAX 256
#define BUFF_SIZE 256

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

void sortListStatus(List *clas){
    List stu1, stu2;
    if(clas->pHead == clas->pTail) return;
    InitList(&stu1);  InitList(&stu2);
    User *p;
    User *tag;
    tag = clas->pHead;
    clas->pHead = clas->pHead->pNext;
    tag->pNext = NULL;
    while(clas->pHead != NULL){
        p = clas->pHead;
        clas->pHead = clas->pHead->pNext;
        p->pNext = NULL;
        if(p->status >= tag->status) {
            addUser(&stu1, p);
        }
        else addUser(&stu2, p);
    }
    sortListStatus(&stu1);
    sortListStatus(&stu2);
    if(stu1.pHead != NULL){
        clas->pHead = stu1.pHead;
        stu1.pTail->pNext = tag;
    }
    else{
        clas->pHead = tag;
    }
    tag->pNext = stu2.pHead;
    if(stu2.pHead != NULL){
        clas->pTail = stu2.pTail;
    }
    else{
        clas->pTail = tag;
    }
}

void sortListWontimes(List *clas){
    List stu1, stu2;
    if(clas->pHead == clas->pTail) return;
    InitList(&stu1);  InitList(&stu2);
    User *p;
    User *tag;
    tag = clas->pHead;
    clas->pHead = clas->pHead->pNext;
    tag->pNext = NULL;
    while(clas->pHead != NULL){
        p = clas->pHead;
        clas->pHead = clas->pHead->pNext;
        p->pNext = NULL;
        if(p->win_times >= tag->win_times) {
            addUser(&stu1, p);
        }
        else addUser(&stu2, p);
    }
    sortListWontimes(&stu1);
    sortListWontimes(&stu2);
    if(stu1.pHead != NULL){
        clas->pHead = stu1.pHead;
        stu1.pTail->pNext = tag;
    }
    else{
        clas->pHead = tag;
    }
    tag->pNext = stu2.pHead;
    if(stu2.pHead != NULL){
        clas->pTail = stu2.pTail;
    }
    else{
        clas->pTail = tag;
    }
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