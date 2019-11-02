#NAME: Erica Xie, Michelle Lam
#EMAIL: ericaxie@ucla.edu, laammichelle@gmail.com
#ID: 404920875, 404926523

CC = gcc
CFLAGS = -Wall -Wextra -g 

default: lab3a.c ext2_fs.h 
	$(CC) $(CFLAGS) lab3a.c -o lab3a

clean:
	rm -f lab3a-404920875.tar.gz lab3a 

dist: 
	tar czf lab3a-404920875.tar.gz Makefile README lab3a.c ext2_fs.h 
