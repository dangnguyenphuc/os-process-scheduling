#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void swap(struct pcb_t *a,struct pcb_t *b)
{
  struct pcb_t temp = *b;
  *b = *a;
  *a = temp;
}

void reHeapUp(struct queue_t * q , int pos){
	if(pos > 0 && pos<q->size){
		int parent = (pos-1)/2;
		if(q->proc[pos]->priority > q->proc[parent]->priority){
			swap((q->proc[parent]), (q->proc[pos]));
			reHeapUp(q,parent);
		}
	}
	return;
}

void reHeapDown(struct queue_t * q, int pos){
	int largest = pos;
    int l = 2 * pos + 1;
    int r = 2 * pos + 2;
    if (l < q->size && q->proc[l]->priority > q->proc[largest]->priority)
        largest = l;
    if (r < q->size && q->proc[r]->priority > q->proc[largest]->priority)
        largest = r;
    if (largest != pos) {
        swap(q->proc[largest], q->proc[pos]);
        reHeapDown(q, largest);
    }
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if(empty(q)){
		q->size+=1;
		q->proc[0] = proc;
	}
	else if(q->size < MAX_QUEUE_SIZE){ 	// if < then insert last current pos
		q->proc[q->size] = proc;	// new process: proc inserted to queue 
		q->size++;					// Update q->size
		reHeapUp(q,q->size);
	}

}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	
	if(q->size == 0) return NULL;
	struct pcb_t* _search = q->proc[0];
	q->proc[0] = q->proc[q->size-1];
	q->size -= 1;
	reHeapDown(q,0);

	return _search;
}

