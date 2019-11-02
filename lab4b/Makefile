#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu
#ID: 404720875

CC = gcc
CFLAGS = -Wall -Wextra -g 

default: lab4b.c 
	$(CC) $(CFLAGS) lab4b.c -o lab4b

check:
	chmod +x test
	./test

clean:
	rm -f lab4b-404920875.tar.gz lab4b

dist: 
	tar czf lab4b-404920875.tar.gz Makefile README lab2_list.c test 
