/*
NAME: Erica Xie
EMAIL: ericaxie@ucla.edu
ID: 404920875
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <pthread.h> 
#include <time.h> 
#include <signal.h> 
#include "SortedList.h" 

/* global variables */
int opt_yield = 0; //yield flag 
int m = 0; //sync mutex flag 
int s = 0; //spin lock flag 
int spin_lock = 0; 
int num_keys = 0; 
int t = 1; //number of threads
pthread_mutex_t lock; 
SortedList_t *list; 
SortedList_t head; 
SortedListElement_t *elements; 

void gen_keys(int num_keys) {
    char num; 
    for(int n = 0; n < num_keys; n++) { 
        char *key = malloc(4*sizeof(char)); 
        num = (char) ('a' + (rand() % 26)); 
        *key = num; 
        num = (char) ('a' + (rand() % 26)); 
        *(key + 1) = num; 
        num = (char) ('a' + (rand() % 26)); 
        *(key + 2) = num; 
        *(key + 3) = '\0'; 
        elements[n].key = key; 
    }
}

void handler() {
	fprintf(stderr, "Segmentation Fault hehe\n");
	exit(2); 
}

void spin() {
    while(__sync_lock_test_and_set(&spin_lock, 1)) {
        //spin
    }
}

void release() {
    __sync_lock_release(&spin_lock); 
}

void *thread_func(void *input) {

    int tid = *(int *)input; 
    int tnum = t; 
    int knum = num_keys; 
    int n = 0; 
    int len = 0; 
    SortedListElement_t *temp; 

    if(m) {
        pthread_mutex_lock(&lock);
    }  
    if(s) {
        spin();
    } 

    //insert each element into the list 
    for(n = tid; n < knum; n += tnum) {  
        SortedList_insert(list, &elements[n]); 
    }

    //find length of list 
    len = SortedList_length(list); 
    if(len == -1) {
        fprintf(stderr, "Length function found corruption\n");
        exit(2); 
    }  

    //lookup & delete each element from the list 
    for(n = tid; n < knum; n += tnum) {
        temp = SortedList_lookup(list, elements[n].key); 
        if(temp == NULL) {
            fprintf(stderr, "Key not found\n");
            exit(2); 
        }
        if(SortedList_delete(temp) == 1) { //free on this side? 
            fprintf(stderr, "Delete function found corruption\n");
            exit(2); 
        }    
    }

    if(m) {
        pthread_mutex_unlock(&lock);
    }  
    if(s) {
        release(); 
    } 

    return NULL; 
}

int main(int argc, char **argv) {

    signal(SIGSEGV, handler);

    /* declare all variables */
    char buf[2048], yields[3], name[50]; 
    int opt, i, n; //t
    long run_time, avg_time, nano;  
    struct timespec start; 
    struct timespec end; 
    long long start_time, end_time;  

    static struct option long_options[] =
    {
        {"threads", optional_argument, NULL, 't'},
        {"iterations", optional_argument, NULL, 'i'},
        {"yield", required_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {0, 0, 0, 0}
    };

    /* initialize variables */ 
    i = 1; //number of iterations 
    n = 0; 
    run_time = 0;
    avg_time = 0;
    nano = 1000000000; 

    list = &head; 
    head.key = NULL; 
    head.prev = list; 
    head.next = list; 

    /* check optional arguments */ 
    while((opt = getopt_long(argc, argv, "tiys:", long_options, NULL)) != -1) {
        switch(opt){
            case 't':
                t = atoi(optarg); 
                break;
            case 'i':
                i = atoi(optarg);
                break;
            case 'y': 
                if(strlen(optarg) > 3) {
                    fprintf(stderr, "Too many inputs to Yield\n");
                    exit(1);
                }
                for(unsigned int it = 0; it < strlen(optarg); it++) {
                    if(*(optarg+it) == 'i') {
                        opt_yield = opt_yield | INSERT_YIELD;
                        yields[it] = 'i'; 
                    }
                    else if(*(optarg+it) == 'd') {
                        opt_yield = opt_yield | DELETE_YIELD; 
                        yields[it] = 'd'; 
                    }
                    else if(*(optarg+it) == 'l') {
                        opt_yield = opt_yield | LOOKUP_YIELD;
                        yields[it] = 'l'; 
                    }
                    else {
                        fprintf(stderr, "Invalid Input to Yield\n");
                        exit(1);
                    }
                }
                break;
            case 's':
                if(strcmp("m", optarg)== 0) 
                    m = 1; 
                else if(strcmp("s", optarg)== 0) 
                    s = 1; 
                else {
                    fprintf(stderr, "Invalid Input to Sync\n");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Invalid Argument\n");
                exit(1);
        }
    }

    /* find name of test */
    if(m) {
        if(opt_yield) 
            sprintf(name, "list-%s-m", yields);
        else 
            sprintf(name, "list-none-m");
    }
    else if (s) {
        if(opt_yield) 
            sprintf(name, "list-%s-s", yields);
        else
            sprintf(name, "list-none-s");
    }
    else if (opt_yield) 
        sprintf(name, "list-%s-none", yields);
    else 
        sprintf(name, "list-none-none");  

    /* create and initializes keys */
    num_keys = t*i; 
    elements = malloc(num_keys*sizeof(SortedListElement_t));
    if(elements == NULL) {
        fprintf(stderr, "Error creating keys: %s\n", strerror(errno)); 
        exit(2);
    } 
    gen_keys(num_keys); 

    /* initialize thread variables */
    pthread_t threads[t]; 
    int thread_ids[t]; 
    int operations = 3*num_keys; 

    /* create lock */
    if(m) {
        if(pthread_mutex_init(&lock, NULL) != 0) {
            fprintf(stderr, "Cannot create lock\n%s\n", strerror(errno));
            exit(2);
        }
    }

    /* start time */
    if(clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
        exit(1);
    } 
    start_time = start.tv_sec*nano + start.tv_nsec; 

    /* create threads */
    for(n = 0; n < t; n++) { 
        thread_ids[n] = n; 
        if (pthread_create(&threads[n], NULL, thread_func, (void *)&thread_ids[n])) {
            fprintf(stderr, "Cannot create threads\n%s\n", strerror(errno));
            exit(2);
        } 
    }

    /* join threads */
    for(n = 0; n < t; n++) {
        if(pthread_join(threads[n], NULL)) { 
            fprintf(stderr, "Cannot join threads\n%s\n", strerror(errno));
            exit(2);
        } 
    }

    int len = SortedList_length(list); 
    if(len == -1) {
        fprintf(stderr, "Length function found corruption\n");
        exit(2); 
    } 
    if(len != 0) {
        fprintf(stderr, "Did not delete all elements\n");
        exit(2); 
    } 

    /* end time */
    if(clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
        fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
        exit(1);
    } 
    end_time = end.tv_sec*nano + end.tv_nsec; 
    run_time = end_time - start_time; 
    avg_time = run_time / operations; 

    /* printing csv */
    sprintf(buf, "%s,%d,%d,%d,%d,%ld,%ld\n", name, t, i, 1, operations, run_time, avg_time);
    if(write(1, &buf, strlen(buf)) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
        exit(1);
    } 

    /* iterate through keys and free all */
    for(int k = 0; k < num_keys; k++) {
        free((void *)elements[k].key);
    }
    free(elements);

}