
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
	/*TODO: get a process from [ready_queue]. If ready queue
	 * is empty, push all processes in [run_queue] back to
	 * [ready_queue] and return the highest priority one.
	 * Remember to use lock to protect the queue.
	 * */

	//lock 'cause ready queue's role likes a global variable.
	pthread_mutex_lock(&queue_lock); // lock ==NULL
	
	if(ready_queue.size == 0){ //then we move all processes in running q into ready q.
		for(int i = 0;i<run_queue.size;++i){
			ready_queue.proc[i] = run_queue.proc[i];
			run_queue.proc[i]=0;
		}
		ready_queue.size = run_queue.size;
		run_queue.size = 0;
	}
	//then just dequeue ready queue whether ready q's size = 0 or not.
	proc = dequeue(&ready_queue);

	//unlock
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


