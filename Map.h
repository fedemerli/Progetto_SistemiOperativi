#ifndef _MAP_HEADER_
#define _MAP_
#define _GNU_SOURCE
#define SO_WIDTH  30
#define SO_HEIGHT 10

typedef enum {FALSE,TRUE} boolean;

typedef struct cell{
	int x;
  	int y;
	int capacity;
  	int timeCross;
  	int cross; /*conta quanti taxi hanno attraversato la cella*/
  	boolean available; /*se la cella è disponibile o no*/
  	boolean source; /*cella che genera richieste*/
}Cell; 

typedef struct city{
	Cell map[SO_HEIGHT][SO_WIDTH];
}City;

typedef struct coordinates{
	long mytype;
	int x;
	int y;
}Coordinates;

extern City* setMap(int minCap, int maxCap, int minTime, int maxTime,int holes,int sources);
extern void printMap(City* city,int semCity);
extern void printFinalMap(City* city, int sources);
extern void request(City* city, int i, int j, int numTaxi, int msgId);
extern int newRequest(City* city,int msgId);

#endif
