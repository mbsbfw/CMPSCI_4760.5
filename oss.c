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

#define MAX_FORKS 18
#define MAX_RAND 5
#define CLOCK_INC 1000

//SigHand protos 
static void myhandler(int s);
static int setupinterrupt(void);
static int setupitimer(void);

void initializeResources();
void createProcess(int pidHolder[]);
void writeToLog(); 
void ossClock();
void checkMsgQ();
void jobProcess(int);
void procDetected(int, int);
void procBlocked(int, int);
void allocated(int, int);
void allocatedTable();
void fAllocatedTable();

int main(int argc, char* argv[]){

	if(setupinterrupt () == -1){
		perror("Failed to set up handler for SIGPROF");
		return 1;
	}//end if

	if (setupitimer() == -1){
		perror("Failed to set up the ITIMER_PROF interval timer");
		return 1;
	}//end if
	
	int option;
	int maxProcs;

	while((option = getopt(argc, argv, "hn:")) != -1){

		switch(option){

			case 'h' :
				printf("\t\t---Help Menu---\n");
				printf("\t-h Display Help Menu\n");
				printf("\t-n x indicate the maximum total of child processes\n");
				exit(1);
			
			case 'n' :
				maxProcs = atoi(optarg);
				if(maxProcs > 20)
					maxProcs = 18;
				break;

			case '?' :
				printf("ERROR: Improper arguments");
				break;

		}//end switch
	}//end while
	
	//setup the shared memory
	getSharedMemory();

	//setup message queue
	messageQueueConfig();

	//initialze resource descriptors and max table
	initializeResources();

	int remainder;

	while(1){
		if ((sysClock_ptr->nanoSecs + CLOCK_INC) > 999999999){
			remainder = (sysClock_ptr->nanoSecs + CLOCK_INC) - 999999999;
			sysClock_ptr->seconds += 1;
			sysClock_ptr->nanoSecs = remainder;
		}else{
			sysClock_ptr->nanoSecs += CLOCK_INC;
		}
		
		createProcess(pidHolder);

		checkMsgQ();
		
		allocatedTable();

	}//end while

	shmdt(sysClock_ptr);
	return 0;
}//end main

static void myhandler(int s){

	char aster = '*';
	int errsave;
	errsave = errno;
	msgctl(msgqid, IPC_RMID, NULL);
	write(STDERR_FILENO, &aster, 1);
	printf("Terminate after 2 REAL life seconds\n");
	//printf("clock:[%d][%d]", myClock[0], myClock[1]);
	exit(1);
	errno = errsave;
}//end myhandler

static int setupinterrupt(void){

	struct sigaction act;
	act.sa_handler = myhandler;
	act.sa_flags = 0;
	return(sigemptyset(&act.sa_mask) || sigaction(SIGPROF, &act, NULL));
}//end setupinterrupt

static int setupitimer(void){

	struct itimerval value;
	value.it_interval.tv_sec  = 2;
	value.it_interval.tv_usec  = 0;
	value.it_value  = value.it_interval;	
	return (setitimer(ITIMER_PROF, &value, NULL));
}//end setupitimer


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

void createProcess(int pidHolder[]){
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

void writeToLog(){
	int i;
	int j;

	FILE *fp = fopen("log.txt", "a+");

	fprintf(fp, "##### MAX TABLE #####\n");
	fprintf(fp, "     ");
	for(j = 0; j < 20; j++){
		fprintf(fp, "R%02i ", j);
	}

	fprintf(fp, "\n");

	for(i = 0; i < 18; i++){
		fprintf(fp, "P%02i:", i);
		for(j = 0; j < 20; j++){
			fprintf(fp, "%4d", resDes_ptr->max[i][j]);
			totalLines++;
		}//end jfor
		fprintf(fp, "\n");
	}//end ifor

	fprintf(fp, "\n##### RESCOURCES #####\n");
	fprintf(fp, "     ");
	for(j = 0; j < 20; j++){
		fprintf(fp, "R%02i ", j);
	}//end for

	fprintf(fp, "\n    ");

	for(j = 0; j < 20; j++){
		fprintf(fp, "%4d", resDes_ptr->rescources[j]);
	}//end for

	fprintf(fp, "\n");

	fprintf(fp, "##### REQUEST TABLE #####\n");
	fprintf(fp, "     ");

	for(j = 0; j < 20; j++){
		fprintf(fp, "R%02i ", j);
	}//end for

	fprintf(fp, "\n");

	for(i = 0; i < 18; i++){
		fprintf(fp, "P%02i:", i);
		for(j = 0; j < 20; j++){
			fprintf(fp, "%4d", resDes_ptr->request[i][j]);
			totalLines++;
		}//end jfor
		fprintf(fp, "\n");
	}//end ifor

	fprintf(fp, "\n");

	fprintf(fp, "##### JOBS #####\n");
	for(i = 0; i < MAX_FORKS; i++){
		fprintf(fp, "%d    ", resDes_ptr->pidJob[i]);
	}//end for

	fprintf(fp, "\n");

	fprintf(fp, "##### TIME INTERVALS #####\n");
	for(i = 0; i < MAX_FORKS; i++){
		fprintf(fp, "%d    ", randomClock[i]);
	}//end for

	fprintf(fp, "\n");

	fprintf(fp, "##### ALLOCATED #####\n");
	fprintf(fp, "     ");
	for(j = 0; j < 20; j++){
		fprintf(fp, "R%02i ", j);
	}//end for

	fprintf(fp, "\n");

	for(i = 0; i < 18; i++){
		fprintf(fp, "P%02i:", i);
		for(j = 0; j < 20; j++){
			fprintf(fp, "%4d", resDes_ptr->allocated[i][j]);
			totalLines++;
		}//end jfor
		fprintf(fp, "\n");
	}//end ifor

	fprintf(fp, "\n");

	fprintf(fp, "##### BLOCKED QUEUE #####\n");
	for(i = 0; i < MAX_FORKS; i++){
		fprintf(fp, "%d    ", blockedQueue[i]);
	}//end for

	fclose(fp);
}// end writeTolog

void checkMsgQ(){
	int passPid;

	msgrcv(msgqid, &message, sizeof(message), 1, IPC_NOWAIT);

	if(message.msgText[0] != '0') {
		passPid = atoi(message.msgText);
		jobProcess(passPid);
	}

	strcpy(message.msgText, "0");
}//end checkMsg

void jobProcess(int pid){
	int i;
	int jobNum;
	int processNum;
	int resReqNum;

	for(i = 0; i < MAX_FORKS; i++){
		if(pidHolder[i] == pid){
			jobNum = resDes_ptr->pidJob[i];
			processNum = i;
			resReqNum = (rand() % 20);
			procDetected(processNum, resReqNum);
		}//end if
	}//end for

	if(jobNum == 1 || jobNum == 2){
		if(resDes_ptr->request[processNum][resReqNum] <= resDes_ptr->rescources[resReqNum]){
			resDes_ptr->allocated[processNum][resReqNum] = resDes_ptr->request[processNum][resReqNum];
			resDes_ptr->rescources[resReqNum] -= resDes_ptr->request[processNum][resReqNum];
			allocated(processNum, resReqNum);
		}else {
			int i;
			int posted = 0;
			for(i = 0; i < MAX_FORKS; i++){
				if(blockedQueue[i] == 0){
					blockedQueue[i] = pid;
					posted = 1;

					procBlocked(processNum, resReqNum);
				}
				if(posted == 1){
					i = MAX_FORKS;
				}
			}//end for
		}
	}else if(jobNum == 0){
		requestTimeReached = 1;
		pidHolder[processNum] = 0;
	}
}//jobProcess

void procDetected(int procNumber, int reqNum){
	
	//open log output file add process detected 	  
	FILE *fp = fopen("log.txt", "a+");
	fprintf(fp, "OS granting P%d request R%d at time %d:%d\n",
	procNumber, reqNum, sysClock_ptr->seconds, sysClock_ptr->nanoSecs);

	fclose(fp);
}//end detected

void procBlocked(int procNumber, int reqNum){

	//open log output file process blocked
	FILE *fp = fopen("log.txt", "a+");
	fprintf(fp, "OS blocking P%d for requesting R%d at time %d:%d\n",
		procNumber, reqNum, sysClock_ptr->seconds, sysClock_ptr->nanoSecs);
	fclose(fp);
}//end blocked

void allocated(int procNumber, int reqNum){

		  
	FILE *fp = fopen("log.txt", "a+");
	fprintf(fp, "OS granting P%d request R%d at time %d:%d\n",
	procNumber, reqNum, sysClock_ptr->seconds, sysClock_ptr->nanoSecs);

	fclose(fp);
}

void allocatedTable(){
	
	int i;
	int j;
	int lines;
	int ch = 0;

	FILE *fp = fopen("log.txt", "a+");
	
	while(!feof(fp)){
		ch = fgetc(fp);
		if(ch == '\n'){
			lines++;
		}//end if
	}//end while

	if(lines % 20 == 0){
		fprintf(fp, "\n");
		
		for(j = 0; j < 20; j++){
			fprintf(fp, "R%02i ", j);
		}//end for
		
		fprintf(fp, "\n");

		for(i = 0; i < MAX_FORKS; i++){
			fprintf(fp, "P%02i:", i);
			for(j = 0; j < 20; j++){
				fprintf(fp, "%4d", resDes_ptr->allocated[i][j]);
				totalLines++;
			}//end jfor
			fprintf(fp, "\n");
		}//end ifor
		fprintf(fp, "\n");
	}//end if

	fclose(fp);
}//end allocTable

void fAllocatedTable(){
	
	int i;
	int j;

	FILE *fp = fopen("log.txt", "a+");
	
	for(j = 0; j < 20; j++){
		fprintf(fp, "R%02i ", j);
	}//end for
	
	fprintf(fp, "\n");

	for(i = 0; i < MAX_FORKS; i++){
		fprintf(fp, "P%02i:", i);
		for(j = 0; j < 20; j++){
			fprintf(fp, "%4d", resDes_ptr->allocated[i][j]);
			totalLines++;
		}//end jfor
		fprintf(fp, "\n");
	}//end ifor
	fprintf(fp, "\n");
	
	fclose(fp);
}




