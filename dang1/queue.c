#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if(q->size<MAX_QUEUE_SIZE)
	{
		q->proc[q->size]=proc;
		q->size++;
	}
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if(q->size!=0)
	{
		struct pcb_t* temp_proc=q->proc[0];
		int max_priority=temp_proc->priority;
		int max_idx=0;
		int i;
		for(i=1;i<q->size;++i)
		{
			if(q->proc[i]->priority>max_priority)
			{
				temp_proc=q->proc[i];
				max_priority=temp_proc->priority;
				max_idx=i;
			}	
		}
		for(i=max_idx;i<q->size-1;++i)
			q->proc[i]=q->proc[i+1];
		q->proc[q->size-1]=NULL;
		--q->size;
		return temp_proc;
	}
	return NULL;
}

