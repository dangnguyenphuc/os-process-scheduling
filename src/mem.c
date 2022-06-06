
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
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
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
			return 1;
		}
	}
	return 0;	
}

// reference from K16 OS

/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
int memory_avail_to_allocate(int num_pages, struct pcb_t* proc){
	//if virtual space >= RAM -> memmory is not available -> return false.
	if(proc->bp + num_pages*PAGE_SIZE >= RAM_SIZE) return 0;

	int i = 0;
	int free_page = 0;
	for(i = 0; i< NUM_PAGES; ++i){
		if(_mem_stat[i].proc == 0){
			if(free_page == num_pages) break;
			free_page += 1;
		}
	}

	//if number of free page < num_page -> mem is not available
	if(free_page < num_pages) return 0;
	//else
	return 1;
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = ((size % PAGE_SIZE) == 0) ? size / PAGE_SIZE : size / PAGE_SIZE + 1; // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */

	//----------------------------------------------------------------------------------------------------
	//CHECK PHYSICAL MEMORY STATE

	int i, num_need_spaces = 0;
	for (i = 0; i < NUM_PAGES; i++)
	{
		if (_mem_stat[i].proc == 0)
		{
			num_need_spaces++;
		}
		if (num_need_spaces == num_pages)
		{
			mem_avail = 1;
			break;
		}
	}
	
	//CHECK VIRTUAL MEMORY
	if (proc->bp + num_pages * PAGE_SIZE > (1 << ADDRESS_SIZE))
		mem_avail = 0;
	//------------------------------------------------------------------------------------------------
	
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		addr_t addr_vir_mem = ret_mem;
		num_need_spaces = 0;
		int segIndex = -1;
		int pageSize = -1;
		int frame_prev = -1;
		for (i=0; i<NUM_PAGES;i++)
		{
			if (_mem_stat[i].proc == 0)
			{
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = num_need_spaces;
				if (frame_prev != -1)
				{
					_mem_stat[frame_prev].next = i;
				}
				
				frame_prev = i;
				segIndex = get_first_lv(addr_vir_mem);
				if (proc->seg_table->table[segIndex].pages == NULL)
				{
					proc->seg_table->table[segIndex].pages = malloc(sizeof(struct page_table_t));
					proc->seg_table->table[segIndex].pages->size = 0;
				}

				proc->seg_table->table[segIndex].pages->size++;
				pageSize = proc->seg_table->table[segIndex].pages->size - 1;

				proc->seg_table->table[segIndex].v_index = segIndex;
				proc->seg_table->table[segIndex].pages->table[pageSize].v_index = get_second_lv(addr_vir_mem);
				proc->seg_table->table[segIndex].pages->table[pageSize].p_index = i;

				addr_vir_mem += PAGE_SIZE;
				num_need_spaces++;
				proc->seg_table->size++;
				if (num_need_spaces == num_pages)
				{
					_mem_stat[i].next = -1;
					break;
				}
			}
			
		}
	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	pthread_mutex_lock(&mem_lock);	
	int num_pages = 0;					// Number of pages we will use
	addr_t physical_addr;
	addr_t virtual_addr = address;
	int i;
	if(translate(address,&physical_addr,proc)){	// check address is valid and get physical_addr
		addr_t physical_page=physical_addr>>OFFSET_LEN;

		while(physical_page!=-1){
			_mem_stat[physical_page].proc=0;
			addr_t segIndex = get_first_lv(virtual_addr);
			for (i = 0; i < proc->seg_table->table[segIndex].pages->size; i++) 
				if (proc->seg_table->table[segIndex].pages->table[i].p_index == physical_page) {
					proc->seg_table->table[segIndex].pages->table[i].v_index = 0;
					proc->seg_table->table[segIndex].pages->table[i].p_index = 0;
			}
			physical_page=_mem_stat[physical_page].next;
			virtual_addr+=PAGE_SIZE;
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