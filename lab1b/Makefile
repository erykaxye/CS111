
#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu
#ID: 404720875

CC = gcc
CFLAGS = -Wall -Wextra -g -lz

default: client server

client: lab1b-client.c
	$(CC) $(CFLAGS) lab1b-client.c -o lab1b-client

server: lab1b-server.c
	$(CC) $(CFLAGS) lab1b-server.c -o lab1b-server

clean:
	rm -f lab1b-404920875.tar.gz lab1b-client lab1b-server

dist:
	tar czf lab1b-404920875.tar.gz lab1b-client.c lab1b-server.c Makefile README
