CC = gcc
FLAGS = -Wall -g -fPIC

all: stnc

stnc: server.o client.o stnc.o
	$(CC) $(FLAGS) -o stnc stnc.o server.o client.o -lssl -lcrypto

stnc.o: stnc.c client.c server.c
	$(CC) $(FLAGS) -c stnc.c

server.o: server.c server.h
	$(CC) $(FLAGS) -c server.c

client.o: client.c client.h
	$(CC) $(FLAGS) -c client.c

.PHONY:clean all

clean:
	rm -f *.o stnc