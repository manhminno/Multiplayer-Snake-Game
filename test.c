#include <stdio.h>
#include <string.h>


char *showRoom(char room[]){
    const char space[2] = "_";
    char *token;
    char tmp[256];
    int i = 1;
    char *main;
    token = strtok(room, space);
    main = token;
    strcpy(tmp, token);
    printf("%d. %s\n", i, tmp);
    i++;
    token = strtok(NULL, space);
    while(token != NULL){
        strcpy(tmp, token);
        printf("%d. %s\n", i, tmp);
        i++;
        token = strtok(NULL, space);
    }
    return main;
}

int main(){
    char tmp[234];
    strcpy(tmp,"_manhnvtest_hsss_manhsss_sss3s_hoang trung_s");
    // printf("%s\n", tmp);
    char *tmp2 = showRoom(tmp);
    printf("%s\n", tmp2);
}
