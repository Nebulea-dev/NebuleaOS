#include "usermode.h"
#include "segment.h"
#include "consts.h"
#include "process.h"

void new_process_start_function()
{
	Process *current_process = process_table[current_pid];
	// printf("Starting process %d\n", current_pid);

	__asm__ __volatile__("pushl %0" : : "r"(USER_DS));							// SS
	__asm__ __volatile__("pushl %0" : : "r"((int)current_process->user_stack)); // ESP
	__asm__ __volatile__("pushl %0" : : "r"(0x202));							// EFLAGS -> Met IOPL à 0
	__asm__ __volatile__("pushl %0" : : "r"(USER_CS));							// CS
	__asm__ __volatile__("pushl %0" : : "r"(ONE_GIGABYTE));						// EIP

	__asm__ __volatile__("movl %0, %%ds" : : "r"(USER_DS));
	__asm__ __volatile__("movl %0, %%es" : : "r"(USER_DS));
	__asm__ __volatile__("movl %0, %%fs" : : "r"(USER_DS));
	__asm__ __volatile__("movl %0, %%gs" : : "r"(USER_DS));

	// __asm__ __volatile__("movl %0, %%ebp" : : "r"((int)current_process->user_stack)); // EBP

	__asm__ __volatile__("iret");
}