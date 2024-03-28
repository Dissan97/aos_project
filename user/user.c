

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "include/types.h"

int main(int argc, char** argv){
	int i = 0;
	syscall(156, "password","/home/dix/padova.c", ADD_PATH | ON);
	syscall(156, "password","/home/dix/padova.c", ADD_PATH | REC_ON);
}
