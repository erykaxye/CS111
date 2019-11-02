/*
NAME: Erica Xie
EMAIL: ericaxie@ucla.edu
ID: 404920875
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <pthread.h> 
#include <time.h> 
#include <signal.h>
#include "SortedList.h" 

/*
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element) { 

    if(list == NULL || element == NULL) {
        return; 
    }

    SortedListElement_t *cur_node = (SortedListElement_t*)(list->next);

    if(opt_yield & INSERT_YIELD) {
        sched_yield(); 
    }

    while(cur_node->key != NULL) {
        // if(opt_yield & INSERT_YIELD) {
        //     sched_yield(); 
        // }
        if(strcmp(element->key, cur_node->key) <= 0) { //<=0
            break; 
        }
        cur_node = cur_node->next;
    }

    element->next = cur_node;
    element->prev = cur_node->prev;
    cur_node->prev->next = element;  
    cur_node->prev = element; 
    return; 

}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete(SortedListElement_t *element) {

    if(opt_yield & DELETE_YIELD) {
        sched_yield(); 
    }

    if(element == NULL) {
        return 1; 
    }

    if(element->prev->next != element->next->prev) 
        return 1; 

    element->prev->next = element->next; 
    element->next->prev = element->prev; 
    return 0; 
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {

    if(list == NULL || key == NULL) {
        return NULL; 
    }

    if(opt_yield & LOOKUP_YIELD) {
        sched_yield(); 
    }

    SortedListElement_t *cur_node = (SortedListElement_t*)(list->next);
    
    while(cur_node->key != NULL) {
        if(strcmp(key, cur_node->key)) {
            return cur_node; 
        }
        cur_node = cur_node->next;
    }
    return NULL; 
}

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list) {

    if(list == NULL) {
        return -1; 
    }

    if(opt_yield & LOOKUP_YIELD) {
        sched_yield(); 
    } 

    int counter = 0; 
    SortedListElement_t *cur_node = (SortedListElement_t*)(list->next);
    
    while(cur_node->key != NULL) {
        if(cur_node->prev->next != cur_node->next->prev) 
            return -1; 
        cur_node = cur_node->next;
        counter++; 
    }
    return counter; 
}



