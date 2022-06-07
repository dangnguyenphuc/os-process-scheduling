#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if (empty(q)) {
		q->proc[0] = proc;
		q->size++;
	} else if (q->size < MAX_QUEUE_SIZE) {
		int index = 0;
		for (int i = 0; i < q->size; i++) {
			if (proc->priority <= q->proc[i]->priority) {
				index++;
			}
		}
		for (int i = q->size; i > index; i--) {
			q->proc[i] = q->proc[i - 1];
		}
		q->proc[index] = proc;
		q->size++;
	}
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	struct pcb_t * tmp = q->proc[0];
	if (q->size == 0) {
		return NULL;
	} else if (q->size == 1) {
		q->proc[0] = NULL;
		q->size--;
	} else {
		for (int i = 0; i < q->size - 1; i++) {
			q->proc[i] = q->proc[i + 1];
		}
		q->size--;
	}
	return tmp;
}