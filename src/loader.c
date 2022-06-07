// WRONG IN LINE: 89
#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t avail_pid = 1;

#define OPT_CALC	"calc"
#define OPT_ALLOC	"alloc"
#define OPT_FREE	"free"
#define OPT_READ	"read"
#define OPT_WRITE	"write"

static enum ins_opcode_t get_opcode(char * opt) {
	if (!strcmp(opt, OPT_CALC)) {
		return CALC;
	}else if (!strcmp(opt, OPT_ALLOC)) {
		return ALLOC;
	}else if (!strcmp(opt, OPT_FREE)) {
		return FREE;
	}else if (!strcmp(opt, OPT_READ)) {
		return READ;
	}else if (!strcmp(opt, OPT_WRITE)) {
		return WRITE;
	}else{
		printf("Opcode: %s\n", opt);
		exit(1);
	}
}

struct pcb_t * load(const char * path) {
	/* Create new PCB for the new process */
	// Cấp phát vùng nhớ cho process ở địa chỉ path
	struct pcb_t * proc = (struct pcb_t * )malloc(sizeof(struct pcb_t));

	// Gán process id cho process được yêu cầu load, bắt đầu từ 1
	proc->pid = avail_pid;
	avail_pid++;

	// Cấp phát vùng nhớ cho seg_table của process
	proc->seg_table =
		(struct seg_table_t*)malloc(sizeof(struct seg_table_t));
	
	// Khởi tạo break pointer cho process, mặc định ban đầu process chỉ được
	// cho 1 page = 1024 bytes
	proc->bp = PAGE_SIZE; // = 1024
	// Khởi tạo program counter cho process = 0 (từ câu lệnh đầu tiên)
	proc->pc = 0;

	/* Read process code from file */
	// Bắt đầu mở file và lấy dữ liệu trong code của process
	FILE * file;
	if ((file = fopen(path, "r")) == NULL) {
		printf("Cannot find process description at '%s'\n", path);
		exit(1);		
	}
	char opcode[10];
	// cấp phát vùng nhớ cho CẢ ĐOẠN CODE của process
	proc->code = (struct code_seg_t*)malloc(sizeof(struct code_seg_t));
	// Lưu độ ưu tiên của process và số câu lệnh trong process
	fscanf(file, "%u %u", &proc->priority, &proc->code->size);
	// Cấp phát vùng nhớ cho mảng các câu lệnh của process
	proc->code->text = (struct inst_t*)malloc(
		sizeof(struct inst_t) * proc->code->size
	);
	uint32_t i = 0;
	// Xử lý từng câu lệnh trong process
	for (i = 0; i < proc->code->size; i++) {
		// Lấy opcode của câu lệnh thứ i
		fscanf(file, "%s", opcode);
		proc->code->text[i].opcode = get_opcode(opcode);

		// Lưu tham số đầu vào của câu lệnh i theo từng trường hợp opcode
		switch(proc->code->text[i].opcode) {
		case CALC:	// 0 tham số
			break;
		case ALLOC:	// 2 tham số
			fscanf(
				file,
				"%u %u\n",
				&proc->code->text[i].arg_0,
				&proc->code->text[i].arg_1
			);
			break;
		case FREE:	// 1 tham số
			fscanf(file, "%u\n", &proc->code->text[i].arg_0);
			break;
		case READ:	// 3 tham số, THẦY VIẾT THIẾU!!!
		case WRITE:	// 3 tham số
			fscanf(
				file,
				"%u %u %u\n",
				&proc->code->text[i].arg_0,
				&proc->code->text[i].arg_1,
				&proc->code->text[i].arg_2
			);
			break;	
		default:
			printf("Opcode: %s\n", opcode);
			exit(1);
		}
	}
	// Trả về con trỏ lưu địa chỉ pcb của process được load
	return proc;
}
