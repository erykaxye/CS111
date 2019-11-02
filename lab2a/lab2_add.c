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

int opt_yield = 0;
int m = 0; 
int s = 0; 
int c = 0; 
int spin_lock = 0;  
pthread_mutex_t lock; 

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield(); 
    *pointer = sum;
}

void spin(void) {
    while(__sync_lock_test_and_set(&spin_lock, 1)) {
        //spin
    }
}

void cas_add(long long *pointer, long long value) {
    long long old, new; 
    do {
        old = *pointer; 
        new = old + value;
        if (opt_yield)
            sched_yield();
    }
    while(old != __sync_val_compare_and_swap(pointer, old, new)); 
}

struct args { 
    long long *pointer; 
    int iterations; 
}; 

void *thread_func(void *arguments) {
    for (int i = 0; i < ((struct args*)arguments)->iterations; i++) {
        if(m)
            pthread_mutex_lock(&lock);
        if(s) 
            spin();
        if(c) {
            cas_add(((struct args*)arguments)->pointer, 1); 
            cas_add(((struct args*)arguments)->pointer, -1); 
        }
        else {
            add(((struct args*)arguments)->pointer, 1); 
            add(((struct args*)arguments)->pointer, -1); 
        }
        if(m)
            pthread_mutex_unlock(&lock);
        if(s) 
            __sync_lock_release(&spin_lock); 
    } 
    return NULL; 
}

int main(int argc, char **argv) {

    /* declare all variables */
    char buf[2048], name[50];
    int opt, i, t, n; 
    long long counter, start_time, end_time; 
    long nano;  
    struct timespec start; 
    struct timespec end; 

    static struct option long_options[] =
    {
        {"threads", optional_argument, NULL, 't'},
        {"iterations", optional_argument, NULL, 'i'},
        {"yield", no_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {0, 0, 0, 0}
    };

    /* initialize variables */
    t = 1; //number of threads
    i = 1; //number of iterations 
    n = 0; 
    counter = 0; 
    nano = 1000000000; 

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
                opt_yield = 1; 
                break;
            case 's':
                if(strcmp("m", optarg)== 0) 
                    m = 1; 
                else if(strcmp("s", optarg)== 0) 
                    s = 1; 
                else if(strcmp("c", optarg)== 0)
                    c = 1; 
                else 
                    fprintf(stderr, "Invalid Input to Sync\n");
                break;
            default:
                fprintf(stderr, "Invalid Argument\n");
                exit(1);
        }
    }

    /* find name of test */
    if(m) {
        if(opt_yield) {
            sprintf(name, "add-yield-m");
        }
        else 
            sprintf(name, "add-m");
    }
    else if (s) {
        if(opt_yield) {
            sprintf(name, "add-yield-s");
        }
        else
            sprintf(name, "add-s");
    }
    else if (c) {
        if(opt_yield) {
            sprintf(name, "add-yield-c");
        }
        else
            sprintf(name, "add-c");
    }
    else if (opt_yield) {
        sprintf(name, "add-yield-none");
    }
    else 
        sprintf(name, "add-none");  

    /* initialize thread variables */
    pthread_t thread_id[t];   
    long operations = 2*t*i; 
    long run_time = 0;
    long avg_time = 0;
    struct args *arguments = (struct args*)malloc(sizeof(struct args)); 
    arguments->pointer = &counter; 
    arguments->iterations = i; 

    /* start time */
    if(clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
        exit(1);
    } 
    start_time = start.tv_sec*nano + start.tv_nsec; 

    /* create lock */
    if(m) {
        if(pthread_mutex_init(&lock, NULL) != 0) {
            fprintf(stderr, "Cannot create lock\n%s\n", strerror(errno));
            exit(2);
        }
    }

    /* create threads */
    for(n = 0; n < t; n++) { 
        if (pthread_create(&thread_id[n], NULL, thread_func, (void *)arguments)) {
            fprintf(stderr, "Cannot create threads\n%s\n", strerror(errno));
            exit(2);
        } 
    } 

    /* join threads */
    for(n = 0; n < t; n++) {
        if(pthread_join(thread_id[n], NULL)) { 
            fprintf(stderr, "Cannot join threads\n%s\n", strerror(errno));
            exit(2);
        } 
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
    sprintf(buf, "%s,%d,%d,%ld,%ld,%ld,%lld\n", name, t, i, operations, run_time, avg_time, counter);
    if(write(1, &buf, strlen(buf)) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
        exit(1);
    } 

    free(arguments); 

    exit(0); 
}


