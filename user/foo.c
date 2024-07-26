#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define TRIES 1000

int main(){
    long states[4] = {0x01, 0x02, 0x04, 0x08};

    int index;
    srand(time(NULL));

    int i = syscall(134, "password", 0x4);
    if (i){
        perror("Something wrong");
    }

    i = syscall(174, "password","/home/dix/aos_project/user/test_dirs", 0x10);
    if (i){
        perror("ADD PATH ISSUE");
    }
    

}