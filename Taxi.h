#define _GNU_SOURCE
#include "Map.h"
#define SIZE_MSG 20

typedef struct taxi{
  int pid;
  Cell position;
  Cell destination;
  float maxTimeTraveled;
  int distanceTraveled;
  int customers;
  boolean passenger;
}Taxi;

typedef struct message{
	long mytype;
	char mex[SIZE_MSG];
}Message;

extern int initializationTaxi(City* city, Taxi* taxi, int semCity, int nTaxi, int msg);
extern void searchSource(Taxi* taxi, Coordinates* sources, int numSource);
extern int goToDestination(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart, int timeOut);