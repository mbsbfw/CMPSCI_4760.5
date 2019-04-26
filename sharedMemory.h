#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

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

#define CSHMKEY 314159
#define RDSHMKEY 31415926

//clock struct
typedef struct{

	unsigned int seconds;
	unsigned int nanoSecs;

}sysClock_t;

//resource descriptor struct
typedef struct{

	pid_t pids[18];
	int pidJob[18];
	int nanosRequest[18];
	int rescources[20];
	int max[18][20];
	int allocated[18][20];
	int request[18][20];
}resDes_t;

//meassage queue struct
struct msgBuffer{
	long msgType;
	char msgText[100];
}message;

/* ALL MY GLOBES */
int sCshmid;
sysClock_t *sysClock_ptr;
int rDshmid;
resDes_t *resDes_ptr;
int totalLines = 0;
int pidHolder[18] = {};
int randomClock[18] = {};
int blockedQueue[18] = {};
int requestTimeReached = 0;
int msgqid;

//allocate shared memory
void getSharedMemory(){
	
	sCshmid = shmget(CSHMKEY, sizeof(sysClock_t), IPC_CREAT|0777);
	if(sCshmid < 0){
		perror("sysClock shmget error in master \n");
		exit(errno);
	}
	
	sysClock_ptr = shmat(sCshmid, NULL, 0);

	if(sysClock_ptr < 0){
		perror("sysClock shmat error in oss\n");
		exit(errno);
	}

	//shared mem for Rescource Descriptor
	rDshmid = shmget(RDSHMKEY, sizeof(resDes_t), IPC_CREAT|0777);

	if(rDshmid < 0){
		perror("RD shmget error in master \n");
		exit(errno);
	}

	resDes_ptr  = shmat(rDshmid, NULL, 0);

	if(resDes_ptr < 0){
		perror("sysClock shmat error in oss\n");
		exit(errno);
	}

}//end getShared

void messageQueueConfig(){

	msgqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
}

#endif
