#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>

#include "sharedMemory.h"

int main(int argc, char* argv[]){
	
	getSharedMemory();

	messageQueueConfig();

	int timeInc;
	int remainder;
	int seconds = sysClock_ptr->seconds;
	int nanoSecs = sysClock_ptr->nanoSecs;

	timeInc = atoi(argv[0]);
	
	//printf("We are in user %d %d\n", getpid(), timeInc);

	if((nanoSecs + timeInc) > 999999999){
		remainder = (nanoSecs + timeInc) - 999999999;
		seconds += 1;
		nanoSecs = remainder;
	}else {
		nanoSecs += timeInc;
	}

	while(requestTimeReached == 0){
		if(sysClock_ptr->seconds >= seconds && sysClock_ptr->nanoSecs>= nanoSecs){
			sprintf(message.msgText, "%d", getpid());

			message.msgType = 1;

			msgsnd(msgqid, &message, sizeof(message), 0);

			requestTimeReached = 1;
		}//end if
	}//end while

	shmdt(sysClock_ptr);
	shmdt(resDes_ptr);
	exit (0);
}//end main
