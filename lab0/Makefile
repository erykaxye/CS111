#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu
#ID: 404720875

CC = gcc 
CFLAGS = -Wall -Wextra -g

default: lab0.c 
	$(CC) $(CFLAGS) lab0.c -o lab0 

check:
	chmod +x smoke-test
	./smoke-test

clean: 
	rm -f lab0-404920875.tar.gz lab0

dist: 
	tar czf lab0-404920875.tar.gz lab0.c Makefile backtrace.png breakpoint.png README smoke-test  
