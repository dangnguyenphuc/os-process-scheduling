#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

//TODO: Search for page table table from the a segment table
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	int i;
	//TODO: Sequential search
	for (i = 0; i < seg_table->size; ++i) {
		// Enter your code here
		// if index found -> get its pages table
		if( index == seg_table->table[i].v_index){
			return seg_table->table[i].pages;
		}
	}
	//else return NULL;
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);	//offset
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr); //segment index
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr); //page index
	
	/* Search in the first level */
	struct page_table_t * page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}
	//else 
	int i;
	for (i = 0; i < page_table->size; ++i) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			addr_t p_index = page_table->table[i].p_index;
			*physical_addr = (p_index << OFFSET_LEN) | (offset);		
			// physical add = 10 bits paging then 10 bits offset
			// so it equals p_index shifts left 10 bits and added by offset
			return 1;
		}
	}
	// if there have no v_index -> 0;
	return 0;	
}

/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE+1 :size / PAGE_SIZE; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?
	/* First we must check if the amount of free memory in
    	* virtual address space and physical address space is
    	* large enough to represent the amount of required
    	* memory. If so, set 1 to [mem_avail].
    	* Hint: check [proc] bit in each page of _mem_stat
    	* to know whether this page has been used by a process.
    	* For virtual memory space, check bp (break pointer).
    	* */

	//--------------- CHECK PHYSICAL MEMORY----------------
	int i;
	int spaces=0;   // If proc = 0, the page is free and the OS could allocated it to any process
	for (int k=0;k<NUM_PAGES;k++){
		if (spaces==num_pages){
			mem_avail=1;
			break;
		}
		else {
			if(_mem_stat[k].proc==0) spaces++;
		}
	}
	//--------------- CHECK VIRTUAL MEMORY----------------
	if (proc->bp+num_pages*PAGE_SIZE>(1<<ADDRESS_SIZE)) {
		mem_avail=0;
	}
	//-------------------------------------------------------

	if (mem_avail) {
		 /* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages*PAGE_SIZE;
		  /* Update status of physical pages which will be allocated
         	* to [proc] in _mem_stat. Tasks to do:
         	*  - Update [proc], [index], and [next] field
         	*  - Add entries to segment table page tables of [proc]
         	*    to ensure accesses to allocated memory slot is
        	*    valid. */
		int pre_frame=0;
		int segSize;
		int pageSize;
		addr_t addr_vir;
		addr_t segIndex;
		addr_t pageIndex;
		for (i=0;i<NUM_PAGES && spaces>0; i++){
			if (_mem_stat[i].proc==0){
				_mem_stat[i].proc=proc->pid;
				_mem_stat[i].index=num_pages-spaces;
				addr_vir=ret_mem + _mem_stat[i].index*PAGE_SIZE;
				

				segIndex=get_first_lv(addr_vir);
				pageIndex=get_second_lv(addr_vir);
				struct page_table_t* page_table=get_page_table(segIndex, proc->seg_table);

				if (page_table==NULL){
					segSize=proc->seg_table->size;
					proc->seg_table->table[segSize].v_index=segIndex;
					proc->seg_table->table[segSize].pages=(struct page_table_t*) malloc(sizeof(struct page_table_t));
					page_table=proc->seg_table->table[segSize].pages;
					page_table->size=0;
					proc->seg_table->size++;
				}	
				pageSize = page_table->size;
				page_table->table[pageSize].v_index=pageIndex;
				page_table->table[pageSize].p_index=i;
				page_table->size++;

				if (spaces<num_pages) _mem_stat[pre_frame].next=i;
				pre_frame = i ;
				spaces-=1;
			}
		}
		_mem_stat[pre_frame].next=-1;
	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
    	* this region is indicated by [address]. Task to do:
    	*    - Set flag [proc] of physical page use by the memory block
    	*      back to zero to indicate that it is free.
    	*    - Remove unused entries in segment table and page tables of
    	*      the process [proc].
    	*    - Remember to use lock to protect the memory from other
    	*      processes.  */
    	int i;
    	int check=0;
    	pthread_mutex_lock(&mem_lock);
		addr_t addr_physical;
		if (translate(address, &addr_physical, proc)){
		// find deleting position
		for (i=0;i<NUM_PAGES;i++){
			if (addr_physical==(i<<OFFSET_LEN)) {
				check=1;
				break;
			}
		}
		int next=-1;

		if (check) {
			next=i;
		}

		addr_t temp=address;
		addr_t segIndex;
		addr_t pageIndex;

		while (next>-1){
			_mem_stat[next].proc=0;
			next=_mem_stat[next].next;
			//Thuc hien xoa 
			segIndex  = get_first_lv(temp);
			pageIndex = get_second_lv(temp);
			for (int j=0;j<proc->seg_table->size;j++){
				if (proc->seg_table->table[j].v_index == segIndex){
					int l;
					for (int k=0;k<proc->seg_table->table[j].pages->size;k++){
						if (pageIndex == proc->seg_table->table[j].pages->table[k].v_index){
						//Xoa phan tu bang cach don cac phan tu tu vi tri xoa roi xoa phan tu cuoi cung cua page_table
							for (l=k;l<proc->seg_table->table[j].pages->size-1;l++){
								proc->seg_table->table[j].pages->table[l].p_index=proc->seg_table->table[j].pages->table[l+1].p_index;
								proc->seg_table->table[j].pages->table[l].v_index=proc->seg_table->table[j].pages->table[l+1].v_index;
							}
							proc->seg_table->table[j].pages->table[l].v_index=0;
							proc->seg_table->table[j].pages->table[l].p_index=0;
							proc->seg_table->table[j].pages->size-=1;
							break;
						}
					}
					//Kiem tra xem page_table trong hay khong, neu trong thi xoa di
					if (proc->seg_table->table[j].pages->size==0){
						free(proc->seg_table->table[j].pages);
						//Don bang seg_table lai roi xoa
						for (l=j;l<proc->seg_table->size-1;l++){
							proc->seg_table->table[l].v_index=proc->seg_table->table[l+1].v_index;
							proc->seg_table->table[l].pages=proc->seg_table->table[l+1].pages;
						}
						proc->seg_table->table[l].pages=NULL;
						proc->seg_table->table[l].v_index=0;
						proc->seg_table->size-=1;
					}
					break;
				}
			}
			temp+=PAGE_SIZE;
		}
	}
	pthread_mutex_unlock(&mem_lock);
	return 0;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}else{
		return 1;
	}
}

int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}

/* Note that:
Number of entries in the page table of a segment = Number of pages that segment is divided.

A segment table exists that keeps track of the frames storing the page tables of segments.

Number of entries in the segment table of a process = Number of segments that process is divided.

The base address of the segment table is stored in the segment table base register.
*/