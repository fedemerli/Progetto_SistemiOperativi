#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include "Taxi.h"

#define TEST_ERROR if (errno) {fprintf(stderr,		\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno)); \
					   if(msgId > 0) msgctl(msgId, IPC_RMID, NULL); \
						if(msgId2 > 0) msgctl(msgId, IPC_RMID, NULL); \
						if(semStart > 0) semctl(semStart,0,IPC_RMID); \
						if(semCity > 0) semctl(semCity,0,IPC_RMID); \
						if(semCityMutex > 0) semctl(semCityMutex,0,IPC_RMID); \
						exit(EXIT_FAILURE);}

void initialization();
void dataCheck();
void cleanAll();
void signal_handler(int);
void startTaxi(int);
void killAllProcess();
void endGame();

int SO_TAXI;
int SO_SOURCES;
int SO_HOLES;
int SO_TOP_CELLS;
int SO_CAP_MIN;
int SO_CAP_MAX;
long SO_TIMESEC_MIN;
long SO_TIMESEC_MAX;
int SO_TIMEOUT;
int SO_DURATION;
City* city = NULL;
Coordinates* sources = NULL;
Taxi taxi ;
Taxi* allTaxis = NULL;
int* processTaxis=NULL;
int* processRequests=NULL;
int semCity=0;
int semCityMutex=0;
int semStart=0;
int msgId=0;
int msgId2=0;
int shmId=0;
int status=0;
int count=0;
int numTaxi=0;
int numTotProcess=0;
int status;
boolean stop;

void signal_handler(int sig){
	int i;

	switch (sig){
		case SIGINT: /*This signal handles ctrl+c*/
			killAllProcess(FALSE);
			cleanAll();
			break;
		case SIGALRM: /*This signal handles the duration of the execution*/
			stop=TRUE;
			killAllProcess(TRUE);
			endGame();
			cleanAll();
			break;
		case SIGUSR1:/*This signal handles the new request(kill -s 10 pid)*/
			newRequest(city,msgId);
			break;
		case SIGUSR2:
			removeTaxi(&taxi, semCity, TRUE);
			break;
	}
}

int main(void){
	
	int son=0,i=0,passenger=0;
	struct sigaction signalHandler;
	Message message;
	size_t sizeOfMessage = sizeof(Message)-sizeof(long);
	
	initialization();

	for(i=0; i<SO_SOURCES; i++){ /*It creates a request's process*/
		switch(son=fork()){
			case -1:
				printf("FORK ERROR\n");
				exit(EXIT_FAILURE);
			case 0:
				request(city, sources[i].x, sources[i].y, SO_TAXI, msgId);
				exit(EXIT_FAILURE);
			default:
				processRequests[i]=son;
				/*printf("Source process number %d forked, with pid = %d \n", i, son);*/
				break;	
		}
	}
	for(i=0; i<SO_TAXI; i++){ /*It creates a taxi's process*/
		switch(son=fork()){
			case -1:
				printf("FORK ERROR\n");
				exit(EXIT_FAILURE);
			case 0:
				startTaxi(i+1);
				exit(EXIT_FAILURE);
			default:
				processTaxis[i]=son;
				/*printf("Taxi process number %d forked, with pid = %d \n", i, son);*/
				break;
		}
	}

	if(son!=0){

		stop = FALSE;
		signalHandler.sa_handler = signal_handler;
		if((sigaction(SIGINT, &signalHandler, NULL))==-1) TEST_ERROR
		if((sigaction(SIGALRM, &signalHandler, NULL))==-1) TEST_ERROR
		if(sigaction(SIGUSR1, &signalHandler, NULL) == -1) TEST_ERROR 

		for(i =0; i< 3;i++){
			printf("\n--------START GAME %d--------\n", 3-i);
			sleep(1);
		}
		alarm(SO_DURATION);

		/*The taxis can be started*/
		for(i=0; i<SO_TAXI+1;i++){
			if(semV(semStart,i)==-1) TEST_ERROR
		} 

		while(!stop){
			
			if(semP(semStart,0)==-1) TEST_ERROR
			printMap(city, semCity);
			if(semV(semStart,0)==-1) TEST_ERROR
			
			while(msgrcv(msgId2, &message, sizeOfMessage, 0,IPC_NOWAIT)>0){ /*A taxi has died */

				if(numTaxi>=SO_TAXI){
					if((allTaxis = realloc(allTaxis, (numTaxi+SO_TAXI)*sizeof(Taxi)))==NULL) TEST_ERROR
				} 

				sscanf(message.mex, "%d %d %d %d %f", &allTaxis[numTaxi].pid, &passenger, 
						&allTaxis[numTaxi].customers, &allTaxis[numTaxi].distanceTraveled, &allTaxis[numTaxi].maxTimeTraveled);

				/*printf("Ricevuto messaggio = %s\n", message.mex);*/
						
				if(passenger==1){
					allTaxis[numTaxi].passenger=TRUE;
				}
				passenger=0;

				switch(son=fork()){
					case -1:
						printf("FORK ERROR\n");
						exit(EXIT_FAILURE);
					case 0:
						startTaxi(numTaxi+SO_TAXI+1);
						exit(EXIT_FAILURE);
					default:
						/*printf("The eliminated taxi %d was replaced by %d\n", allTaxis[numTaxi].pid, son);*/
						for(i=0; i<SO_TAXI; i++){
							if(processTaxis[i]==allTaxis[numTaxi].pid){
								processTaxis[i]=son;
							}
						}
						numTaxi++;
						break;
				}
			}
			sleep(1);
		}
	}
	exit(EXIT_SUCCESS);
}

void initialization(){

	int i, j, k;
	pid_t son;

	printf("inserisci numeri di SO_TAXI ");
	scanf("%d", &SO_TAXI);
	printf("Inserisci il numero di centri richieste SO_SOURCES ");
	scanf("%d", &SO_SOURCES);
	printf("Inserisci il numero di zone inacessibili SO_HOLES ");
	scanf("%d", &SO_HOLES);
	printf("Il massimo di tempo che un processo puo' rimanere fermo in una cella SO_TIMEOUT ");
	scanf("%d", &SO_TIMEOUT);
	printf("Tempo massimo di attraversamento di una cella SO_TIMESEC_MAX ");
	scanf("%ld", &SO_TIMESEC_MAX);
	printf("Tempo minimo di attraversamento di una cella SO_TIMESEC_MIN ");
	scanf("%ld", &SO_TIMESEC_MIN);
	printf("Capacita' massima di ogni cella: SO_CAP_MAX ");
	scanf("%d", &SO_CAP_MAX);
	printf("Capacita' minima di ogni cella: SO_CAP_MIN ");
	scanf("%d", &SO_CAP_MIN);
	printf("Numero di celle maggiormente attraversate: SO_TOP_CELLS ");
	scanf("%d", &SO_TOP_CELLS);
	printf("Durata simulazione: SO_DURATION ");
	scanf("%d", &SO_DURATION);
	
	dataCheck();

	printf("\nThe master pid for signal is : %d\n",getpid());

	if((processRequests = malloc(sizeof(int)*SO_SOURCES))==NULL) TEST_ERROR
	if((processTaxis = malloc(sizeof(int)*SO_TAXI))==NULL) TEST_ERROR
	if((sources = malloc(sizeof(Coordinates)*SO_SOURCES))==NULL) TEST_ERROR
	if((allTaxis = malloc(sizeof(Taxi)*SO_TAXI))==NULL) TEST_ERROR

	city = setMap(SO_CAP_MIN, SO_CAP_MAX, SO_TIMESEC_MIN, SO_TIMESEC_MAX,SO_HOLES,SO_SOURCES);
	semCity = initSem(city, FALSE); /*It create a semaphore for each cell*/
	semCityMutex = initSem(city, TRUE);/*It create a traffic light for each Cell to prevent taxis from misspelling*/
	semStart = semget(IPC_PRIVATE, SO_TAXI+1, IPC_CREAT | 0600);/*It create semaphore to make every taxi start at the same time*/
	for(i=0;i<SO_TAXI+1;i++){
		if(semctl(semStart, i, SETVAL, 0)) TEST_ERROR /*Initialize to 0 so every taxi starts asleep*/
	}
	if((msgId = msgget(getpid(), IPC_CREAT|IPC_EXCL|0660)) == -1) TEST_ERROR
	if((msgId2 = msgget(getpid()+1, IPC_CREAT|IPC_EXCL|0660)) == -1) TEST_ERROR

	for(i=0; i<SO_HEIGHT;i++){/*Save the sources cells in the array*/
		for(j=0;j<SO_WIDTH;j++){
			if(city->map[i][j].source){
				sources[k].x = city->map[i][j].x;
				sources[k].y = city->map[i][j].y;
				k++;
			}
		}
	}
}

void dataCheck(){

	if(SO_SOURCES < 1 || SO_TOP_CELLS < 0 || SO_CAP_MIN < 0 || SO_CAP_MAX <= 0 || SO_CAP_MAX <= 0 
	|| SO_TIMEOUT <= 0 || SO_DURATION <= 0 || SO_TAXI<= 0 || SO_HOLES < 0 || SO_TIMESEC_MIN < 0 || SO_TIMESEC_MAX <= 0 ){
		printf("Error: in Parameters, check every parameter and set a possible value\n"); /* parameters check */
		exit(EXIT_FAILURE);
	}			
	if( SO_TOP_CELLS>(SO_HEIGHT*SO_WIDTH)){
		printf("Error, invalid SO_TOP_CELLS value\n");
		exit(EXIT_FAILURE);
	}
	if(SO_TIMESEC_MIN>SO_TIMESEC_MAX){
		printf("Error, the minimum timensec cannot be greater than the maximum\n");
		exit(EXIT_FAILURE);
	}
	if(SO_CAP_MIN>SO_CAP_MAX){
		printf("Error, the minimum cell capacity cannot be greater than the maximum\n");
		exit(EXIT_FAILURE);
	}
	if(SO_SOURCES+SO_HOLES > SO_HEIGHT*SO_WIDTH){
		printf("Error, too many sources, retrun the program with less sources %d \n",SO_SOURCES);
		exit(EXIT_FAILURE);
	}
}

void cleanAll(){
	if(msgctl(msgId, IPC_RMID, NULL)==-1) TEST_ERROR
	if(msgctl(msgId2, IPC_RMID, NULL)==-1) TEST_ERROR
	if(semctl(semCity, 0, IPC_RMID)==-1) TEST_ERROR
	if(semctl(semStart, 0, IPC_RMID)==-1) TEST_ERROR
	if(semctl(semCityMutex, 0, IPC_RMID)==-1) TEST_ERROR
	shmdt(city->map);
}

void killAllProcess(boolean term){
	int i;

	for(i=0;i<SO_SOURCES;i++){
		kill(processRequests[i],SIGTERM);
	}
	if(term){
		for(i=0;i<SO_TAXI;i++){
			/*printf("Mando un SIGUSR2 al processo %d, numero %d\n", processTaxis[i], i);*/
			kill(processTaxis[i],SIGUSR2);
		}
	}else{
		for(i=0;i<SO_TAXI;i++){
			/*printf("Mando una SIGTERM al processo %d\n", processTaxis[i]);*/
			kill(processTaxis[i],SIGTERM);
		}
	}
	while(wait(NULL)!=-1);
}

void startTaxi(int num){
	Coordinates coordinate;
	struct sigaction signalHandler;

	bzero(&signalHandler, sizeof(signalHandler));
	signalHandler.sa_handler = signal_handler;
	if((sigaction(SIGUSR2, &signalHandler, NULL))==-1){
   		fprintf(stderr,"Error %s:%d: in initTaxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
 	}

	if(initializationTaxi(city,&taxi,semCity,SO_TAXI,msgId2)==-1) TEST_ERROR

	/*Every taxi will go toghether*/
	if(num<SO_TAXI+1){
		if(semP(semStart, num)==-1) TEST_ERROR
	}

	while(TRUE){
		
		taxi.passenger=FALSE;
		searchSource(&taxi, sources, SO_SOURCES);

		if(goToDestination(city, &taxi, semCity, semCityMutex, semStart,SO_TIMEOUT)==-1) TEST_ERROR

		if(msgrcv(msgId, &coordinate, sizeof(int)*2, semNum(taxi.position)+1,0)>0){

			/*printf("Taxi %d has receved a request with destination %d %d\n", getpid(),coordinate.x,coordinate.y);*/
			taxi.destination.x = coordinate.x;
			taxi.destination.y = coordinate.y;
			taxi.passenger=TRUE;
			if(goToDestination(city, &taxi, semCity, semCityMutex, semStart, SO_TIMEOUT)==-1) TEST_ERROR
			/*printf("Taxi %d compleate a request !!!\n", getpid());*/
		}
	}
}

void endGame(){

	int passenger=0,totalTravels=0, i=0, inevasi=0,aborted=0;
	Taxi* bestTaxiTime;
	Taxi* bestTaxiTrip;
	Taxi* bestTaxiCustomers;
	Coordinates coordinates;
	Message message;
	size_t sizeOfMessage = sizeof(Message)-sizeof(long);

	printf("\n-------------GAME OVER!------------\n\n");

	while(msgrcv(msgId2, &message, sizeOfMessage, 0,IPC_NOWAIT)>0){

		if(numTaxi>=SO_TAXI){
			if((allTaxis = realloc(allTaxis, (numTaxi+SO_TAXI)*sizeof(Taxi)))==NULL) TEST_ERROR
		} 

		sscanf(message.mex, "%d %d %d %d %f", &allTaxis[numTaxi].pid, &passenger, 
						&allTaxis[numTaxi].customers, &allTaxis[numTaxi].distanceTraveled, &allTaxis[numTaxi].maxTimeTraveled);

		if(passenger==1) {
			allTaxis[numTaxi].passenger=TRUE;
		}
		passenger = 0;
		numTaxi++;
	}
	while(msgrcv(msgId, &coordinates,sizeof(int)*2, 0,IPC_NOWAIT) >= 0){
		inevasi++;
	}

	for(i=0;i<(numTaxi+SO_TAXI);i++){
		if(allTaxis[i].passenger==TRUE){
			aborted++;
		}
		totalTravels = totalTravels+allTaxis[i].customers;
	}

	bestTaxiTrip = &allTaxis[0];
	bestTaxiTime = &allTaxis[0];
	bestTaxiCustomers = &allTaxis[0];

	for(i=1;i<(numTaxi+SO_TAXI);i++){
		if(allTaxis[i].distanceTraveled>bestTaxiTrip->distanceTraveled){
			bestTaxiTrip = &allTaxis[i];
		}
		if(allTaxis[i].maxTimeTraveled>bestTaxiTime->maxTimeTraveled){
			bestTaxiTime = &allTaxis[i];
		}
		if(allTaxis[i].customers>bestTaxiCustomers->customers){
			bestTaxiCustomers = &allTaxis[i];
		}
	}

	printFinalMap(city,SO_TOP_CELLS);
	printf("The taxi that made the longest trip was %d, with %d cells\n", bestTaxiTrip->pid, bestTaxiTrip->distanceTraveled);
	printf("The taxi that made the longest journey in serving a request was %d, with %f seconds\n", bestTaxiTime->pid, bestTaxiTime->maxTimeTraveled);
	printf("The taxi that served the most customers was %d, with %d customers\n", bestTaxiCustomers->pid, bestTaxiCustomers->customers);
	printf("\n TRIPS COMPLETED SUCCESSFULLY: %d \n", totalTravels);
	printf("\n ABORTED TRIPS: %d \n", aborted);
	printf("\n UNANSWERED REQUESTS: %d \n", inevasi);
}












