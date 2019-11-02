#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu
#ID: 404720875

CC = gcc
CFLAGS = -Wall -Wextra -g -lmraa -lm

default: lab4c_tcp.c lab4c_tls.c
	$(CC) $(CFLAGS) lab4c_tcp.c -o lab4c_tcp 
	$(CC) $(CFLAGS) -lssl -lcrypto lab4c_tls.c -o lab4c_tls

clean:
	rm -f lab4c-404920875.tar.gz lab4c_tcp lab4c_tls

dist:
	tar czf lab4c-404920875.tar.gz Makefile README lab4c_tcp.c lab4c_tls.c

