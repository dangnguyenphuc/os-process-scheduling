#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if(q->size < MAX_QUEUE_SIZE){ 	// if < then insert last current pos
		q->proc[q->size] = proc;	// new process: proc inserted to queue 
		q->size++;					// Update q->size
	}
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if(q->size>0){
		struct pcb_t* search=NULL;
		int search_priority = -1;
		int search_index = -1;
		int i;
		for(i=0; i<q->size; ++i){
			if(q->proc[i]->priority > search_priority){
				search = q->proc[i];
				search_priority = q->proc[i]->priority;
				search_index = i;
			}
		}
		// move up index if search_index<q->size-1
		// else just size = size -1
		for(i = search_index ; i<q->size-1; ++i )
			q->proc[search_index] = q->proc[search_index+1];
		
		//then 
		q->proc[q->size-1]=NULL;
		q->size -= 1;
		return search;
	}
	return NULL;
}

