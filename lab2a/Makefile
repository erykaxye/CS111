#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu
#ID: 404720875

CC = gcc
CFLAGS = -Wall -Wextra -g -lpthread 

default: build

build: lab2_add.c lab2_list.c SortedList.c SortedList.h
	$(CC) $(CFLAGS) lab2_add.c -o lab2_add
	$(CC) $(CFLAGS) lab2_list.c SortedList.c -o lab2_list  

tests: 
	chmod +x test1
	chmod +x test2
	./test1
	./test2

graphs:
	chmod +x lab2_add.gp
	chmod +x lab2_list.gp
	./lab2_add.gp
	./lab2_list.gp

clean:
	rm -f lab2a-404920875.tar.gz lab2_add lab2_list

dist: default tests graphs 
	tar czf lab2a-404920875.tar.gz Makefile README lab2_add.c lab2_list.c SortedList.h SortedList.c lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png test1 test2 lab2_add.gp lab2_list.gp
