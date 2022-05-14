
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
	for (i = 0; i < seg_table->size; i++) {
		// Enter your code here
		if( index == seg_table->table[i].v_index){
			return seg_table->table[i].pages;
		}
	}
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
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			*physical_addr = offset + page_table->table[i].p_index * PAGE_SIZE ;
			return 1;
		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = (size % PAGE_SIZE) ? (size / PAGE_SIZE) : (size / PAGE_SIZE + 1); // Number of pages we will use
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */
	int i;
	int number_of_available_page = 0;
	for(i=0 ; i<NUM_PAGES; ++i ){
		if(_mem_stat[i].proc == 0){
			number_of_available_page += 1;
			if(number_of_available_page == num_pages && (proc->bp + num_pages * PAGE_SIZE) <= RAM_SIZE){
				mem_avail = 1;
				break;
			}
		}
	}
	
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

		int number_of_page_alloc = 0;
		int prev_page;					// index of previos page
		addr_t cur_vadd;				// current virtual address;
		
		addr_t seg_index;
		addr_t page_index;

		int offset;
		for(i=0; i<NUM_PAGES; ++i){
			if(_mem_stat[i].proc == 0){
				_mem_stat[i].proc = proc->pid;
				_mem_stat[i].index = number_of_page_alloc;

				if(_mem_stat[i].index != 0){
					_mem_stat[prev_page].next = i;
				}
				//Update previos page index:
				prev_page = i;

				//flag found:
				int flag = 0;
				struct seg_table_t* seg_table = proc->seg_table;
				if(seg_table->table[0].pages == NULL){
					seg_table->size == 0;
				}

				//calc current logical address:
				cur_vadd = ret_mem + (number_of_page_alloc<<OFFSET_LEN);
				//then we can get page index and segment index
				seg_index = get_first_lv(cur_vadd);
				page_index = get_second_lv(cur_vadd);

				for(int j = 0; seg_table->size; ++j){
					if(seg_table->table[j].v_index == seg_index){
						struct page_table_t* cur_page_tab = seg_table->table[j].pages;
						//Insert last
						cur_page_tab->table[cur_page_tab->size].v_index = page_index;
						cur_page_tab->table[cur_page_tab->size].p_index = i;
						//Update size
						cur_page_tab->size += 1;
						//then found=1
						flag = 1;
						break;
					}
				}

				if(!flag){ 	//If not found, add new row into table 
					seg_table->table[seg_table->size].v_index = seg_index;
					seg_table->table[seg_table->size].pages = (struct page_table_t *)malloc(sizeof(struct page_table_t));

					seg_table->table[seg_table->size].pages->table[0].v_index = page_index;
					seg_table->table[seg_table->size].pages->table[0].p_index = i;

					seg_table->size++;
				}
				number_of_page_alloc += 1;
				if(number_of_page_alloc==num_pages){
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

	//lock
	pthread_mutex_lock(&mem_lock);
	struct page_table_t * page_table;
	page_table = get_page_table(	get_first_lv(address), 	// get index
									proc->seg_table);		// get proc's segtable
	
	int valid = 0;
	if(page_table != NULL){	// if NULL return 1;
		for(int i = 0; i<page_table->size; ++i){
			if(page_table->table[i].v_index==get_second_lv(address)){ 	// if equal then keep running else i++
				addr_t physical_add;
				if(translate(address, &physical_add,proc)){ 			// translate into physical address if translation fails break for loop
					// physical_add now has its value;
					int index = physical_add>>OFFSET_LEN;			//shift right OFFSET_LEN bit to get "m-n" high bit.
					int num_available_page = 0;
					addr_t current_vadd;
					addr_t seg_index;
					addr_t page_index;

					current_vadd = address + (num_available_page<<OFFSET_LEN);
					while (index != -1)
					{
						_mem_stat[index].proc = 0;

						//flag found
						int flag = 0;
						seg_index = get_first_lv(current_vadd);
						page_index = get_second_lv(current_vadd);

						for(int j=0; j < proc->seg_table->size && !flag; ++j){
							if(proc->seg_table->table[j].v_index == seg_index){
								for(int k=0; k < proc->seg_table->table[j].pages->size; ++k){
									if(proc->seg_table->table[j].pages->table[k].v_index == page_index){
										for(int l = k; l < proc->seg_table->table[j].pages->size-1; k++){
											//in this step we just rearrange page table by moving position by 1
											proc->seg_table->table[j].pages->table[l] = proc->seg_table->table[j].pages->table[l+1];
										}
										//Adjust size;
										proc->seg_table->table[j].pages->size -= 1;
										if(proc->seg_table->table[j].pages->size){
											free(proc->seg_table->table[j].pages);
											for(int l = j; l < proc->seg_table->size-1; ++l){
												//in this step we just rearrange segment table by moving position by 1
												proc->seg_table->table[l] = proc->seg_table->table[l+1];
											}
											//adjust segtable size;
											proc->seg_table->size -= 1;
										}
										flag = 1;
										break;
									}
								}
							}
						}

						index = _mem_stat[index].next;
						num_available_page += 1;

					}
					// Done! valid = 1;
					valid = 1;
				}
				break;
			}
		}
	}
	//unlock
	pthread_mutex_unlock(&mem_lock);	

	//check valid to return 
	if(valid){
		return 0;
	}
	else{
		return 1;
	}
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


