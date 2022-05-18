CFLAGS = -std=c89 -pedantic -g -lm

game : Master.c Function.c Request.c Taxi.c Map.c Map.h Taxi.h
	gcc  $(CFLAGS) Master.c Function.c Request.c Taxi.c Map.c -o game

master: Master.c Function.c Taxi.h
	gcc  $(CFLAGS) Master.c
	
taxi: Taxi.c Function.c Taxi.h
	gcc  $(CFLAGS) Taxi.c

map: Map.c Function.c Map.h
	gcc  $(CFLAGS) Map.c
	
clean:
	rm -f game
	rm -f master
	rm -f taxi
	rm -f map

run:
	./game
