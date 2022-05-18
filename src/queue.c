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
	
	if(q->size == 0) return NULL;
	
	int search = 0;

	for(int i = 1; i < q->size; ++i){
		if(q->proc[i]->priority < q->proc[search]->priority){
			search = i;
		}
	}

	struct pcb_t* _search = q->proc[search];
	q->proc[search] = q->proc[q->size - 1];
	q->size -= 1;
	return _search;
}

