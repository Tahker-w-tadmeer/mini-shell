#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
using namespace std;
char *buff;
//making file

char *current_directory=getcwd(buff,200);
void print_errors(int error,char *dir){
    if (error!=0){
        char *err=strcpy("cd faild for ",dir);
        perror(err);
        exit(1);
    }
    current_directory=getcwd(current_directory,200);
    printf("current directory %s",current_directory);
    exit(2);
}

int cd_execute(int args,char * dir){
    if(args==0){
        int err=chdir("/home");
        print_errors(err,"/home");
    }else{
        int err=chdir(dir);
        print_errors(err,dir);
    }
}
