#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Map.h"

void request(City* city, int i, int j, int numTaxi, int msgId){

	boolean find=FALSE;
	Coordinates message;
    int x=0, y=0, w=0;
	
	do{
		srand(getpid());
		while(!find){ 
			x = rand()%SO_HEIGHT; 
			y = rand()%SO_WIDTH; 
				
			if (x!=i && y!=j && city->map[x][y].available){
				find = TRUE;
			}
		}
		message.mytype=semNum(city->map[i][j])+1;
		message.x=x; /* x and y are cordinate of destination's cells */
		message.y=y;
		if(msgsnd(msgId , &message, sizeof(int)*2, 0)==-1){
			fprintf(stderr,"Error %s:%d: in newRequest %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
			exit(EXIT_FAILURE);
		}	
		/*printf("Process %d has generate a new request sulla coda %d\n",getpid(), msgId);*/
		find = FALSE;
		sleep(5);
	}while(!find);
}

int newRequest(City* city,int msgId){
	
	int x,y;
	boolean find = FALSE;
	Coordinates message;

	while(!find){ 
		x = rand()%SO_HEIGHT; 
		y = rand()%SO_WIDTH; 
				
		if (city->map[x][y].available){
			find = TRUE;
		}
	}
	message.mytype=semNum(city->map[x][y])+1;
	message.x=x; /* x and y are cordinate of destination's cells */
	message.y=y;
	if(msgsnd(msgId , &message, sizeof(int)*2, 0)==-1){
		fprintf(stderr,"Error %s:%d: in newRequest %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("A new request has send correctly!\n");
}