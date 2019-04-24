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
#define MAX_FORKS 2
#define MAX_RAND 5
#define CLOCK_INC 1000

void initializeResources();
void forkProcess(int pidHolder[]); 
void ossClock();

int main(int argc, char* argv[]){

	//setup the shared memory
	getSharedMemory();

	//setup message queue
	messageQueueConfig();

	//initialze resource descriptors
	initializeResources();
	int remainder;

	if ((sysClock_ptr->nanoSecs + CLOCK_INC) > 999999999){
		remainder = (sysClock_ptr->nanoSecs + CLOCK_INC) - 999999999;
		sysClock_ptr->seconds += 1;
		sysClock_ptr->nanoSecs = remainder;
	}else{
		sysClock_ptr->nanoSecs += CLOCK_INC;
	}

	forkProcess(pidHolder);

	return 0;
}//end main

void initializeResources(){
	int i;
	int j;
	int randomNum;
	time_t init;

	srand((unsigned) time(&init));

	for(i = 0; i < MAX_FORKS; i++){
		for(j = 0; j < 20; j++){
			randomNum = (rand() % MAX_RAND) + 1;
			resDes_ptr->max[i][j] = randomNum;
		}//end jfor
	}//end ifor
	
	for(i = 0; i < 20; i++){
		randomNum = (rand() % 10) + 1;
		resDes_ptr->rescources[i] = randomNum;
	}//end for
}//end initResource

void forkProcess(int pidHolder[]){
	int i;
	int j;
	int randPercent;

	for(i = 0; i < MAX_FORKS; i++){
		if(pidHolder[i] == 0){
			for(j = 0; j < 20; j++){
				resDes_ptr->request[i][j] = (rand() % MAX_RAND) + 1;
			}//end for

			randPercent = (rand() % 100) + 1; 

			if(randPercent >= 91){
				resDes_ptr->pidJob[i] = 0;
			}

			else if (randPercent >= 60){ 
				resDes_ptr->pidJob[i] = 1;
			}else{ 
				resDes_ptr->pidJob[i] = 2;
			}
			
			randomClock[i] = (rand() % 500000000) + 1000000;

			char buffer[10];

			sprintf(buffer, "%d", randomClock[i]);

			if ((pidHolder[i] = fork()) == 0) {
				execl("./user", buffer, NULL);
			}
		}//end if
	}//end for
}//end forkProc


