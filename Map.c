#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stddef.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <math.h>
#include <time.h>
#include "Map.h"


boolean placeHoles(City* city, int i, int j, int holes){

	int it, jt;
	if (holes == 0){
		return TRUE;
	}
	else{
		boolean trovato = FALSE;
		int x = rand() % SO_HEIGHT;
		int y = rand() % SO_WIDTH;

		for (it = x; trovato == FALSE && it < SO_HEIGHT + x; it++){
			for (jt = y; trovato == FALSE && jt < SO_WIDTH + y; jt++){

				i = it % SO_HEIGHT;
				j = jt % SO_WIDTH;
				if ((i - 1 < 0 || j - 1 < 0 || city->map[i - 1][j - 1].available == TRUE) && (j - 1 < 0 || city->map[i][j - 1].available == TRUE) && (i + 1 == SO_HEIGHT || j - 1 < 0 || city->map[i + 1][j - 1].available == TRUE) &&
					(i - 1 < 0 || city->map[i - 1][j].available == TRUE) && (i + 1 == SO_HEIGHT || city->map[i + 1][j].available == TRUE) && (i - 1 < 0 || j + 1 == SO_WIDTH || city->map[i - 1][j + 1].available == TRUE) &&
					(j + 1 == SO_WIDTH || city->map[i][j + 1].available == TRUE) && (i + 1 == SO_HEIGHT || j + 1 == SO_WIDTH || city->map[i + 1][j + 1].available == TRUE) && city->map[i][j].available == TRUE)
				{

					city->map[i][j].available = FALSE;
					trovato = (FALSE || placeHoles(city, i, j, holes - 1));
					if (trovato == FALSE){
						city->map[i][j].available = TRUE;
					}
				}
			}
		}
		return trovato;
	}
}

boolean correctHoles(City* city, int holes){

	int i = 0;
	int j = 0;
	int x, y, count_i, count_j;
	boolean trovato = FALSE;
	srand(time(NULL));
	x = rand() % SO_HEIGHT;
	srand(time(NULL) + 1);
	y = rand() % SO_WIDTH;

	for (count_i = x; trovato == FALSE && count_i < SO_HEIGHT + x; count_i++){
		for (count_j = y; trovato == FALSE && count_j < SO_WIDTH + y; count_j++){
			i = count_i % SO_HEIGHT;
			j = count_j % SO_WIDTH;
			trovato = placeHoles(city, i, j, holes);
		}
	}
	return trovato;
}

void sourcesGenerator(City* city, int sources){
	int i, j;
	time_t t;

	srand(time(NULL)+getpid());

	while (sources > 0){
		i = rand() % SO_HEIGHT;
		j = rand() % SO_WIDTH;

		if (city->map[i][j].available && !city->map[i][j].source){
			city->map[i][j].source = TRUE;
			sources--;
		}
	}
}

City* setMap(int minCap, int maxCap, int minTime, int maxTime,int holes,int sources){
	
	int shmId,i,j;
	time_t t;
	City* city;

	srand(time(NULL));

	if((shmId = shmget(getpid(),sizeof(City),IPC_CREAT|IPC_EXCL|0666))==-1){
		fprintf(stderr,"Error %s:%d: in AllocateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	if((city = shmat(shmId, NULL, 0))==NULL){ 
    	fprintf(stderr,"Error %s:%d: in AllocateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
   	 	exit(EXIT_FAILURE);
   	}
  	
	for (i = 0; i < SO_HEIGHT; i++){
		for (j = 0; j < SO_WIDTH; j++){
		
			city->map[i][j].x = i;
			city->map[i][j].y = j;
			city->map[i][j].available = TRUE;
			city->map[i][j].source = FALSE;

			if((maxCap - minCap)==0) city->map[i][j].capacity = minCap;
			else city->map[i][j].capacity = (rand() % (maxCap - minCap)) + minCap;
			city->map[i][j].timeCross = (rand() % (maxTime - minTime)) + minTime;
			city->map[i][j].cross = 0;
		}
	}
	if (correctHoles(city ,holes) == FALSE){
		printf("It's not possible inserted %d holes in this city.\n", holes);
		kill(getppid(), SIGINT);
		exit(EXIT_FAILURE);
	}
	sourcesGenerator(city ,sources);
	return city;
}