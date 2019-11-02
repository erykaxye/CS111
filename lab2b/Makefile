#NAME: Erica Xie
#EMAIL: ericaxie@ucla.edu
#ID: 404720875

CC = gcc
CFLAGS = -Wall -Wextra -g -lpthread -lprofiler

default: lab2_list.c SortedList.c SortedList.h
	$(CC) $(CFLAGS) lab2_list.c SortedList.c -o lab2_list

tests:
	chmod +x test
	@-./test 2>/dev/null

profile: lab2_list
	-rm -f ./raw.gperf
	LD_PRELOAD=/usr/local/cs/gperftools-2.7/lib/libprofiler.so.0
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=thread_func ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf 

graphs:
	chmod +x lab2_list.gp
	./lab2_list.gp

clean:
	rm -f lab2b-404920875.tar.gz lab2_list

dist: default tests graphs
	tar czf lab2b-404920875.tar.gz Makefile README lab2_list.c SortedList.h SortedList.c lab2b_list.csv lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png test test lab2_list.gp profile.out

