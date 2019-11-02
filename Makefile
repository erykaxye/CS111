#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu
#ID: 404720875
#CHANGEEEEE

CC = gcc
CFLAGS = -Wall -Wextra -g

default: lab1a.c
	$(CC) $(CFLAGS) lab1a.c -o lab1a

clean:
	rm -f lab1a-404920875.tar.gz lab1a

dist:
	tar czf lab1a-404920875.tar.gz lab1a.c Makefile README 
