#define _GNU_SOURCE
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include "Taxi.h"

struct sembuf sem_op;
struct timespec timeCross;
boolean end;
int numTaxi;
int msgId2;

int initializationTaxi(City* city, Taxi* taxi, int semCity, int nTaxi, int msg){

    int x, y;
	struct sigaction signalHandler;

	numTaxi = nTaxi;
	msgId2 = msg;

    srand(time(NULL)+getpid());
	while(TRUE){/*It generates the coordinates of position of new taxi*/
		x = (rand() % SO_HEIGHT);
        y = (rand() % SO_WIDTH);
		if(city->map[x][y].available){
			if(semctl(semCity,semNum(city->map[x][y]), GETVAL) > 0){
				taxi->position = city->map[x][y];
				if(semP(semCity, semNum(taxi->position))==-1){
        			fprintf(stderr,"Error %s:%d: in taxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        			return -1;
   				}
				break;/*It finds the coordinates*/
			}
		}
	}
    taxi->destination = city->map[0][0];
	taxi->customers = 0;
	taxi->distanceTraveled = 0;
	taxi->maxTimeTraveled = 0;
	taxi->passenger = FALSE;  
	return 0;
}   

void searchSource(Taxi* taxi, Coordinates* sources, int numSource){

    int i; 
    int positionX = taxi->position.x;
    int positionY = taxi->position.y;
    
	srand(time(NULL)+getpid());
    i = rand()%numSource;

	if(positionX != sources[i].x || positionY != sources[i].y){
			
		taxi->destination.x = sources[i].x;
		taxi->destination.y = sources[i].y;
    }else{
		taxi->destination.x = sources[i+1].x;
		taxi->destination.y = sources[i+1].y;
	} 

}

int goToDestination(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart, int timeOut){

    struct timespec start, stop;
	long nTime;
	float sTime;

    if(taxi->passenger==TRUE){

        clock_gettime(CLOCK_REALTIME, &start);
        if(travel(city, taxi, semCity, semCityMutex, semStart, timeOut)==-1){
            fprintf(stderr,"Error %s:%d: in goToDestination %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
        }
        clock_gettime(CLOCK_REALTIME, &stop);

        nTime = stop.tv_nsec-start.tv_nsec;
		sTime = nTime/ pow(10,9);

		if(sTime > taxi->maxTimeTraveled){
			taxi->maxTimeTraveled = sTime;
		}
        taxi->customers++;

    }else{

		/*printf("The taxi %d is into goToDestination in position= %d %d go to destination= %d %d\n", getpid(), taxi->position.x,taxi->position.y,taxi->destination.x,taxi->destination.y);*/
        if(travel(city, taxi, semCity, semCityMutex, semStart, timeOut)==-1){
            fprintf(stderr,"Error %s:%d: in goToDestination %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
        }
		/*printf("Viaggio finito ora sono qui! cordinate x=%d y=%d\n", taxi->position.x, taxi->position.y);*/
    }
    return 0;
}

int accessToUp(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart){
	
	Cell* position = &(city->map[taxi->position.x][taxi->position.y]);
  	Cell* newPosition = &(city->map[taxi->position.x-1][taxi->position.y]);
	struct timespec timeCross;
	timeCross.tv_sec=0;
	timeCross.tv_nsec=city->map[taxi->position.x][taxi->position.y].timeCross;
	
	/*printf("-------------AccessToUp-----------\n");*/
	nanosleep(&timeCross,NULL);

	if(semP(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}

	position->cross++;

	if(semV(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}	

	if(semP(semStart, 0)==-1){/*We use it to avoid printing with data inconsistency*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	if(semV(semCity, semNum(*position))==-1){/*It increments the semaphore of the last position of taxi*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	taxi->position = *newPosition;

	if(semV(semStart, 0)==-1){
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}
	 
	taxi->distanceTraveled++;
	return 0;
}

int accessToDown(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart){
	
	Cell* position = &(city->map[taxi->position.x][taxi->position.y]);
  	Cell* newPosition = &(city->map[taxi->position.x+1][taxi->position.y]);
	struct timespec timeCross;
	timeCross.tv_sec=0;
	timeCross.tv_nsec=city->map[taxi->position.x][taxi->position.y].timeCross;

	/*printf("-------------AccessToDown-----------\n");*/
	nanosleep(&timeCross,NULL);
	
	if(semP(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}

	position->cross++;

	if(semV(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}	

	if(semP(semStart, 0)==-1){/*We use it to avoid printing with data inconsistency*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	if(semV(semCity, semNum(*position))==-1){/*It increments the semaphore of the last position of taxi*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	taxi->position = *newPosition;
	
	if(semV(semStart, 0)==-1){
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	taxi->distanceTraveled++;
	return 0;
}

int accessToRight(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart){
	
	Cell* position = &(city->map[taxi->position.x][taxi->position.y]);
  	Cell* newPosition = &(city->map[taxi->position.x][taxi->position.y+1]);
	struct timespec timeCross;
	timeCross.tv_sec=0;
	timeCross.tv_nsec=city->map[taxi->position.x][taxi->position.y].timeCross;

	/*printf("-------------AccessToRight-----------\n");*/
	nanosleep(&timeCross,NULL);
	
	if(semP(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}

	position->cross++;

	if(semV(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}	

	if(semP(semStart, 0)==-1){/*We use it to avoid printing with data inconsistency*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	if(semV(semCity, semNum(*position))==-1){/*It increments the semaphore of the last position of taxi*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}

	taxi->position = *newPosition;
	
	if(semV(semStart, 0)==-1){
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	taxi->distanceTraveled++; 
	return 0;
}

int accessToLeft(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart){
	
	Cell* position = &(city->map[taxi->position.x][taxi->position.y]);
  	Cell* newPosition = &(city->map[taxi->position.x][taxi->position.y-1]);
	struct timespec timeCross;
	timeCross.tv_sec=0;
	timeCross.tv_nsec=city->map[taxi->position.x][taxi->position.y].timeCross;

	/*printf("-------------AccessToLeft-----------\n");*/  
	nanosleep(&timeCross,NULL);

	if(semP(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}

	position->cross++;

	if(semV(semCityMutex, semNum(*position))==-1){
		fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}	

	if(semP(semStart, 0)==-1){/*We use it to avoid printing with data inconsistency*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	if(semV(semCity, semNum(*position))==-1){/*It increments the semaphore of the last position of taxi*/
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}

	taxi->position = *newPosition;
	
	if(semV(semStart, 0)==-1){
    	fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}

	taxi->distanceTraveled++; 
	return 0;
}

int removeTaxi(Taxi* taxi, int semCity, boolean endGame){

	int passenger=0;
	Message message;

	if(semV(semCity, semNum(taxi->position))==-1){
		fprintf(stderr,"Error %s:%d: in removeTaxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
	}
	
	sprintf(message.mex, "%d %d %d %d %f \n", getpid(), passenger, taxi->customers, taxi->distanceTraveled, taxi->maxTimeTraveled);
	message.mytype = 1;

	/*Send message to the Master whit the taxi's information*/
	if(msgsnd(msgId2,&message,sizeof(Message)-sizeof(long),0)<0){
    	fprintf(stderr,"Error %s:%d: in removeTaxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    	return -1;
 	}
	/*printf("Processo taxi pid = %d terminato correttamente!\n", getpid());*/
	exit(EXIT_SUCCESS);
}

int travel(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart, int timeOut){

    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
	timeCross.tv_nsec = 0;
	timeCross.tv_sec = timeOut;

    while(TRUE){
		
        if (taxi->position.x < taxi->destination.x){ 
			if(city->map[taxi->position.x+1][taxi->position.y].available){								
							
				sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
				if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

                    if(removeTaxi(taxi, semCity, FALSE)==-1){
						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        return -1;
					}

				}else{
  					if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
                        fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        return -1;
                    }
				}
            }else{
				
				if(taxi->position.y < taxi->destination.y){

					sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
					if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

						
						if(removeTaxi(taxi, semCity, FALSE)==-1){
							fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
						}
		
					}else{
						if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                        	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
                    	}
					}

				}else if(taxi->position.y > taxi->destination.y){
					sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
					if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

						
						if(removeTaxi(taxi, semCity, FALSE)==-1){
							fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
						}
			
					}else{
						if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
                        	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
                		}
					}
				}else{
					
					if(goAround(city, taxi, semCity, semCityMutex, semStart)==-1){
                        fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        return -1;
                	}
					
				}					
			}
		}else if (taxi->position.x > taxi->destination.x){	
			if(city->map[taxi->position.x-1][taxi->position.y].available){
				sem_op.sem_num = semNum(city->map[taxi->position.x-1][taxi->position.y]);
				if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

					
					if(removeTaxi(taxi, semCity, FALSE)==-1){
						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                    	return -1;
					}	
						
				}else{
					if(accessToUp(city, taxi, semCity, semCityMutex, semStart)==-1){
                        fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        return -1;
                	}
				}
			}else{
				if(taxi->position.y < taxi->destination.y){

					sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
					if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

					
						if(removeTaxi(taxi, semCity, FALSE)==-1){
							fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
						}
						
					}else{
						if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                        	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
                    	}
					}

				}else if(taxi->position.y > taxi->destination.y){
					sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
					if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

				
						if(removeTaxi(taxi, semCity, FALSE)==-1){
							fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
						}
						
					}else{
						if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
                        	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
                		}
					}
				}else{
					
					if(goAround(city, taxi, semCity, semCityMutex, semStart)==-1){
                        fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        return -1;
                	}

				}
			}
		}else if (taxi->position.x==taxi->destination.x){	
			if (taxi->position.y < taxi->destination.y){	
				if(city->map[taxi->position.x][taxi->position.y+1].available){
					sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
					if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

					
						if(removeTaxi(taxi, semCity, FALSE)==-1){
							fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
						}
						
					}else{
						if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                        	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
                		}
					}
				}else{
					
					if(goAround(city, taxi, semCity, semCityMutex, semStart)==-1){
                        fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        return -1;
                	}
				}
			}else if (taxi->position.y > taxi->destination.y){	
				if(city->map[taxi->position.x][taxi->position.y-1].available){
					sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
					if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){
						
			
						if(removeTaxi(taxi, semCity, FALSE)==-1){
							fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        		return -1;
						}
							
					}else{
						if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
                        	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        	return -1;
                		}
					}
				}else{

					if(goAround(city, taxi, semCity, semCityMutex, semStart)==-1){
                        fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
                        return -1;
                	}
				}
			}
		}
		if(taxi->position.x==taxi->destination.x && taxi->position.y==taxi->destination.y){
			break;
		}
	}
	return 0;
}


int goAround(City* city, Taxi* taxi, int semCity, int semCityMutex, int semStart){

		if(taxi->position.x==taxi->destination.x){
			if(taxi->position.x==0){
				sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
				if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){
					
					if(removeTaxi(taxi, semCity, FALSE)==-1){
						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            			return -1;
					}
					
				}else{
					if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
                    	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            			return -1;
	   				}
					if(taxi->position.y < taxi->destination.y){
						sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){
							
							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            					return -1;
							}
							
						}else{
							if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                    			fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            					return -1;
	   						}
							sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){
																
								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            						return -1;
								}
								
							}else{								
								if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                    				fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            						return -1;
	   							}				
								sem_op.sem_num = semNum(city->map[taxi->position.x-1][taxi->position.y]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){	
																
									if(removeTaxi(taxi, semCity, FALSE)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            							return -1;
									}
									
								}else{
									if(accessToUp(city, taxi, semCity, semCityMutex, semStart)==-1){
                    					fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            							return -1;
	   								}

								}
							}
						}
					}else{
						sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){
													
							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        						return -1;
							}
							
						}else{			
							if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
                   				fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            					return -1;
	   						}
							sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){	

														
								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        							return -1;
								}
								
							}else{				
								if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
                   					fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            						return -1;
	   							}
								sem_op.sem_num = semNum(city->map[taxi->position.x-1][taxi->position.y]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){
																
									if(removeTaxi(taxi, semCity, FALSE)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        								return -1;
									}
									
								}else{				
									if(accessToUp(city, taxi, semCity, semCityMutex, semStart)==-1){
                   						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            							return -1;
	   								}
								}
							}
						}
					}
				}
			}else{
				sem_op.sem_num = semNum(city->map[taxi->position.x-1][taxi->position.y]);
				if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){		
										
					if(removeTaxi(taxi, semCity, FALSE)==-1){
						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        				return -1;
					}
					
				}else{				
					if(accessToUp(city, taxi, semCity, semCityMutex, semStart)==-1){
                		fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            			return -1;
	   				}
					if(taxi->position.y<taxi->destination.y){
						sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){	
													
							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        						return -1;
							}
							
						}else{			
							if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                   				fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            					return -1;
	   						}
							sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

															
								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        							return -1;
								}
								
							}else{	
								if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                   					fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            						return -1;
	   							}	
								sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){	

															
									if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        							return -1;
									}
									
								}else{				
									if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
                						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            							return -1;
	   								}
								}
							}
						}
					}else{
						sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

														
							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        						return -1;
							}
							
						}else{			
							if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
                   				fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            					return -1;
	   						}
							sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){	

													
								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
								
							}else{			
								if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
                   					fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            						return -1;
	   							}	
								sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

									
									if(removeTaxi(taxi, semCity, FALSE)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
									
								}else{
									if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
								}

							}
						}
					}
				}
			}	
		}else if(taxi->position.y==taxi->destination.y){
			if(taxi->position.y==0){
				sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
				if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){
					
								
					if(removeTaxi(taxi, semCity, FALSE)==-1){
						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            			return -1;
					}
					
				}else{							
					if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
                    	fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            			return -1;
	   				}
					if(taxi->position.x<taxi->destination.x){
						sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

							
							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}
							
						}else{
							if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}					
							sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

								
								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
								
							}else{
								if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
								sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

									
									if(removeTaxi(taxi, semCity, FALSE)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
									
								}else{
									if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
								}
							}
						}
					}else{
						sem_op.sem_num = semNum(city->map[taxi->position.x-1][taxi->position.y]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

							
							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}
							
						}else{
							if(accessToUp(city, taxi, semCity, semCityMutex, semStart)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}
							sem_op.sem_num = semNum(city->map[taxi->position.x-1][taxi->position.y]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

								
								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
								
							}else{
								if(accessToUp(city, taxi, semCity, semCityMutex, semStart)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
								sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

									
									if(removeTaxi(taxi, semCity, FALSE)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
									
								}else{
									if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
								}
							}
						}
					}
				}
			}else{
				sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
				if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

					
					if(removeTaxi(taxi, semCity, FALSE)==-1){
						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
						return -1;
					}
					
				}else{
					if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
						fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
						return -1;
					}
					if(taxi->position.x<taxi->destination.x){
						sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}
						
						}else{
							if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}
							sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

							
								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
							
							}else{
								if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
								sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y-1]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

									if(removeTaxi(taxi, semCity, FALSE)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
								
								}else{
									if(accessToLeft(city, taxi, semCity, semCityMutex, semStart)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
								}	
							}
						}
					}else{
						sem_op.sem_num = semNum(city->map[taxi->position.x-1][taxi->position.y]);
						if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

							if(removeTaxi(taxi, semCity, FALSE)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}
						
						}else{
							if(accessToUp(city, taxi, semCity, semCityMutex, semStart)==-1){
								fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
								return -1;
							}
							sem_op.sem_num = semNum(city->map[taxi->position.x+1][taxi->position.y]);
							if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

								if(removeTaxi(taxi, semCity, FALSE)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
							
							}else{
								if(accessToDown(city, taxi, semCity, semCityMutex, semStart)==-1){
									fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
									return -1;
								}
								sem_op.sem_num = semNum(city->map[taxi->position.x][taxi->position.y+1]);
								if((semtimedop(semCity ,&sem_op, 1, &timeCross))<0){

									if(removeTaxi(taxi, semCity, FALSE)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
								
								}else{
									if(accessToRight(city, taxi, semCity, semCityMutex, semStart)==-1){
										fprintf(stderr,"Error %s:%d: in travel %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
										return -1;
									}
								}	
							}
						}		
					}
				}
			}
		}
	return 0;
}

