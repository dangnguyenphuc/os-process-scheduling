#include "queue.h"
#include "sched.h"
#include <pthread.h>

static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

int queue_empty(void) {
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;

	pthread_mutex_lock(&queue_lock);

	if (empty(&ready_queue)) { // Neu ready queue rong
		struct pcb_t * back = NULL;
		while (!empty(&run_queue)) {
			back = dequeue(&run_queue);
			enqueue(&ready_queue, back);
		}
	}
	
	if (!empty(&ready_queue)) {
		proc = dequeue(&ready_queue);
	}

	pthread_mutex_unlock(&queue_lock);

	return proc;
}

void put_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}