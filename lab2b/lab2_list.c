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
int t = 1; //number of threads
int i = 1; //number of iterations 
int l = 1; //number of sublists 
int num_keys = 0; 
long nano = 1000000000; 
long long lock_run_time = 0; 
int *spin_locks; 
pthread_mutex_t *m_locks; 
SortedList_t *lists; 
SortedListElement_t *elements; 
int *list_num; 

void gen_keys() {
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

void gen_lists() {
    lists = malloc(l*sizeof(SortedList_t)); 
    for(int n = 0; n < l; n++) {
        lists[n].key = NULL; 
        lists[n].prev = &lists[n]; 
        lists[n].next = &lists[n]; 
    }
}

void gen_locks() {
    int n = 0; 
    if(m) {
        m_locks = malloc(l*sizeof(pthread_mutex_t)); 
        for(n = 0; n < l; n++) {
            if(pthread_mutex_init(&m_locks[n], NULL) != 0) {
                fprintf(stderr, "Cannot create lock\n%s\n", strerror(errno));
                exit(2);
            }
        }
    }
    if(s) {
        spin_locks = malloc(l*sizeof(int)); 
        for(n = 0; n < l; n++) {
            spin_locks[n] = 0; 
        }
    }
}

int get_hash(const char *key) {
    int hash = *key; // + *(key + 1) + *(key + 2); //not sure if this works?? 
    return hash % l; 
}

void split_keys() {
    list_num = malloc(num_keys*sizeof(int)); 
    for(int n = 0; n < num_keys; n++) {
        list_num[n] = get_hash(elements[n].key); 
    } 
}

void handler() {
	fprintf(stderr, "Segmentation Fault hehe\n");
	exit(2); 
}

void spin(int n) {
    while(__sync_lock_test_and_set(&spin_locks[n], 1)) {
        //spin
    }
}

void release(int n) {
    __sync_lock_release(&spin_locks[n]); 
}

void *thread_func(void *input) {

    int tid = *(int *)input; 
    int tnum = t; 
    int knum = num_keys; 
    int n = 0; 
    int len = 0; 
    int sub_len = 0; 
    SortedListElement_t *temp; 
    struct timespec lock_start; 
    struct timespec lock_end;  
    long long lock_start_time, lock_end_time;  

    //insert each element into the list 
    for(n = tid; n < knum; n += tnum) {  
        if(m) {
            /* start time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_start) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_start_time = lock_start.tv_sec*nano + lock_start.tv_nsec; 

            pthread_mutex_lock(&m_locks[list_num[n]]);

            /* end time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_end) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_end_time = lock_end.tv_sec*nano + lock_end.tv_nsec; 
            lock_run_time += (lock_end_time - lock_start_time); 

            SortedList_insert(&lists[list_num[n]], &elements[n]); 

            pthread_mutex_unlock(&m_locks[list_num[n]]); 
        } 
        else if(s) {
            /* start time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_start) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_start_time = lock_start.tv_sec*nano + lock_start.tv_nsec; 

            spin(list_num[n]); 

            /* end time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_end) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_end_time = lock_end.tv_sec*nano + lock_end.tv_nsec; 
            lock_run_time += (lock_end_time - lock_start_time); 

            SortedList_insert(&lists[list_num[n]], &elements[n]); 

            release(list_num[n]); 
        } 
        else 
            SortedList_insert(&lists[list_num[n]], &elements[n]); 
    }

    //find length of list (or sublist) 
    for(n = 0; n < l; n++) {
        if(m) {
            /* start time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_start) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_start_time = lock_start.tv_sec*nano + lock_start.tv_nsec; 

            pthread_mutex_lock(&m_locks[n]);

            /* end time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_end) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_end_time = lock_end.tv_sec*nano + lock_end.tv_nsec; 
            lock_run_time += (lock_end_time - lock_start_time); 

            sub_len = SortedList_length(&lists[n]); 
            if(sub_len == -1) {
                fprintf(stderr, "Length function found corruption\n");
                exit(2); 
            } 
            len += sub_len; 

            pthread_mutex_unlock(&m_locks[n]); 
        } 
        else if(s) {
            /* start time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_start) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_start_time = lock_start.tv_sec*nano + lock_start.tv_nsec; 

            spin(n); 

            /* end time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_end) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_end_time = lock_end.tv_sec*nano + lock_end.tv_nsec; 
            lock_run_time += (lock_end_time - lock_start_time); 

            sub_len = SortedList_length(&lists[n]); 
            if(sub_len == -1) {
                fprintf(stderr, "Length function found corruption\n");
                exit(2); 
            } 
            len += sub_len; 
            
            release(n); 
        } 
        else {
            sub_len = SortedList_length(&lists[n]); 
            if(sub_len == -1) {
                fprintf(stderr, "Length function found corruption\n");
                exit(2); 
            } 
            len += sub_len; 
        }  
    }

    //lookup & delete each element from the list 
    for(n = tid; n < knum; n += tnum) {   
        if(m) {
            /* start time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_start) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_start_time = lock_start.tv_sec*nano + lock_start.tv_nsec; 

            pthread_mutex_lock(&m_locks[list_num[n]]);

            /* end time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_end) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_end_time = lock_end.tv_sec*nano + lock_end.tv_nsec; 
            lock_run_time += (lock_end_time - lock_start_time); 

            temp = SortedList_lookup(&lists[list_num[n]], elements[n].key); 
            if(temp == NULL) {
                fprintf(stderr, "Key not found\n");
                exit(2); 
            }
            if(SortedList_delete(temp) == 1) { //free on this side? 
                fprintf(stderr, "Delete function found corruption\n");
                exit(2); 
            }  

            pthread_mutex_unlock(&m_locks[list_num[n]]); 
        } 
        else if(s) {
            /* start time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_start) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_start_time = lock_start.tv_sec*nano + lock_start.tv_nsec; 

            spin(list_num[n]); 

            /* end time */
            if(clock_gettime(CLOCK_MONOTONIC, &lock_end) == -1) {
                fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
                exit(1);
            } 
            lock_end_time = lock_end.tv_sec*nano + lock_end.tv_nsec; 
            lock_run_time += (lock_end_time - lock_start_time); 

            temp = SortedList_lookup(&lists[list_num[n]], elements[n].key); 
            if(temp == NULL) {
                fprintf(stderr, "Key not found\n");
                exit(2); 
            }
            if(SortedList_delete(temp) == 1) { //free on this side? 
                fprintf(stderr, "Delete function found corruption\n");
                exit(2); 
            } 
            
            release(list_num[n]); 
        } 
        else {
            temp = SortedList_lookup(&lists[list_num[n]], elements[n].key); 
            if(temp == NULL) {
                fprintf(stderr, "Key not found\n");
                exit(2); 
            }
            if(SortedList_delete(temp) == 1) { //free on this side? 
                fprintf(stderr, "Delete function found corruption\n");
                exit(2); 
            } 
        }
    }

    return NULL; 
}

int main(int argc, char **argv) {

    signal(SIGSEGV, handler);

    /* declare all variables */
    char buf[2048], yields[3], name[50]; 
    int opt, n, len, sub_len; 
    long run_time, avg_time;  
    struct timespec start; 
    struct timespec end; 
    long long start_time, end_time;  

    static struct option long_options[] =
    {
        {"threads", optional_argument, NULL, 't'},
        {"iterations", optional_argument, NULL, 'i'},
        {"yield", required_argument, NULL, 'y'},
        {"sync", required_argument, NULL, 's'},
        {"lists", required_argument, NULL, 'l'}, 
        {0, 0, 0, 0}
    };

    /* initialize variables */ 
    n = 0; 
    run_time = 0;
    avg_time = 0;  
    len = 0; 
    sub_len = 0; 

    /* check optional arguments */ 
    while((opt = getopt_long(argc, argv, "tiysl:", long_options, NULL)) != -1) {
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
            case 'l':
                l = atoi(optarg);
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

    /* create and initialize keys */
    num_keys = t*i; 
    elements = malloc(num_keys*sizeof(SortedListElement_t));
    if(elements == NULL) {
        fprintf(stderr, "Error creating keys: %s\n", strerror(errno)); 
        exit(2);
    } 
    gen_keys(num_keys); 

    /* create and initialize locks */
    if(m || s) {
        gen_locks(); 
    } 

    /* create and initialize sublists */ 
    gen_lists(); 
    split_keys(); 

    /* initialize thread variables */
    pthread_t threads[t]; 
    int thread_ids[t]; 
    int operations = 3*num_keys;  

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

    /* end time */
    if(clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
        fprintf(stderr, "Cannot initialize clock_gettime\n%s\n", strerror(errno));
        exit(1);
    } 
    end_time = end.tv_sec*nano + end.tv_nsec; 
    run_time = end_time - start_time; 
    avg_time = run_time / operations; 
    lock_run_time = lock_run_time / num_keys; 

    /* check that lists are empty */
    for(n = 0; n < l; n++) {
        sub_len = SortedList_length(&lists[n]); 
        if(len == -1) {
            fprintf(stderr, "Length function found corruption\n");
            exit(2); 
        } 
        len += sub_len;  
    }
    if(len != 0) {
        fprintf(stderr, "Did not delete all elements\n");
        exit(2); 
    } 

    /* printing csv */ 
    sprintf(buf, "%s,%d,%d,%d,%d,%ld,%ld,%lld\n", name, t, i, l, operations, run_time, avg_time, lock_run_time);
    if(write(1, &buf, strlen(buf)) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
        exit(1);
    }

    /* iterate through keys and free all */
    for(int k = 0; k < num_keys; k++) {
        free((void *)elements[k].key);
    }
    free(elements); 
    free(spin_locks); 
    free(m_locks); 
    free(lists); 

}