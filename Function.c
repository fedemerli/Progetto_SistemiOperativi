#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>
#include "Map.h"

int semNum(Cell c){
  return (c.x * SO_WIDTH) + c.y;
}

int semP (int semId, int index){
    struct sembuf sem_op;
    sem_op.sem_num  = index;
    sem_op.sem_op   = -1;
    sem_op.sem_flg = 0;

    if((semop(semId, &sem_op, 1))==-1){
      fprintf(stderr,"Error %s:%d: in semP %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
      return -1;
    }
    return 0;
}

int semV(int semId , int index){
	  struct sembuf sem_op;
	  sem_op.sem_num= index;
	  sem_op.sem_op= 1;
	  sem_op.sem_flg= 0;
	
	  if((semop(semId, &sem_op, 1))==-1){
       fprintf(stderr,"Error %s:%d: in semV %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
       return -1;
    }
    return 0;
}

int initSem (City* city, boolean mutex){
  int i,j,ret;
  int height = SO_HEIGHT;
  int width = SO_WIDTH;

  if((ret = semget(IPC_PRIVATE, height*width , IPC_CREAT | 0600))==-1){
    fprintf(stderr,"Error %s:%d: in initSem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
  }
  for(i = 0; i < height; i++){
    for(j = 0; j < width; j++){
      if(mutex){ 
        if((semctl(ret, semNum(city->map[i][j]), SETVAL, 1))==-1){
          fprintf(stderr,"Error %s:%d: in initSem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          exit(EXIT_FAILURE);
        }
      }else {
        if((semctl(ret, semNum(city->map[i][j]), SETVAL, city->map[i][j].capacity))==-1){
          fprintf(stderr,"Error %s:%d: in initSem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          exit(EXIT_FAILURE);
        }
      }
    }
  }
  return ret;
}

void printMap(City* city,int semCity){
	int i,j;

	for(i = 0; i < SO_HEIGHT; i++){
		for(j = 0; j < SO_WIDTH; j++){
      
      int numTaxi = city->map[i][j].capacity - semctl(semCity, semNum(city->map[i][j]), GETVAL);

			if(!city->map[i][j].available){
				printf("[/] ");
			}else{
				printf("[%d] ", numTaxi);
			}
		}
		printf("\n");
	}
  printf("\n");
}

void selection_sort(Cell* arr, int lunghezza) {
    int i,j,max;
    Cell tmp;

    for (i = 0; i < lunghezza-1; i++){

        max = i;

        for (j = i + 1; j < lunghezza; j++){
            if (arr[j].cross > arr[max].cross) max = j;
		    }

        tmp = arr[i];
        arr[i] = arr[max];
        arr[max] = tmp;
      }
}

void printFinalMap(City* city, int sources){
  int i,j,x;
  Cell* topCells = malloc(SO_HEIGHT*SO_WIDTH*sizeof(Cell));
  boolean ok;

  x=0;
  for(i = 0; i < SO_HEIGHT; i++){
		for(j = 0; j < SO_WIDTH; j++){
      topCells[x]=city->map[i][j];
      x++;
    }
  }

  selection_sort(topCells, SO_HEIGHT*SO_WIDTH);

  printf("The source cells are marked of a [S]\n\n");

	for(i = 0; i < SO_HEIGHT; i++){
		for(j = 0; j < SO_WIDTH; j++){
      
        if(city->map[i][j].source){
			  	printf("[S] ");
        }else {
           printf("[/] "); 
        }
    }
		printf("\n");
	}

  printf("\n--------  TOP CELLS  --------\n\n");

  for(i = 0; i<sources; i++){
    printf("Cell %d -> x : %d, y : %d \n", i, topCells[i].x, topCells[i].y);
  }
  printf("\n");
  free(topCells);
}