#include "stdio.h"
#include "cpu.h"
#include "debug.h"

#include "syscall_kernel.h"
#include "syscall_nums.h"

#include "process.h"
#include "stdio.h"
#include "console.h"
#include "message_queue.h"
#include "time.h"
#include "info.h"
#include "screen.h"
#include "semaphore.h"
#include "shared_memory.h"
#include "segment.h"
#include "keyboard.h"

#define CONS_READ_LINE 1

int valid_user_address(void *address);

int __syscall(void)
{
	// get parameter of syscalls
	int syscall_num;
	void *param1;
	void *param2;
	void *param3;
	void *param4;
	void *param5;

	__asm__ __volatile__("movl %%eax, %0" : "=r"(syscall_num));
	__asm__ __volatile__("movl %%ebx, %0" : "=r"(param1));
	__asm__ __volatile__("movl %%ecx, %0" : "=r"(param2));
	__asm__ __volatile__("movl %%edx, %0" : "=r"(param3));
	__asm__ __volatile__("movl %%esi, %0" : "=r"(param4));
	__asm__ __volatile__("movl %%edi, %0" : "=r"(param5));

	__asm__ __volatile__("movl %0, %%ds" : : "r"(KERNEL_DS));
	__asm__ __volatile__("movl %0, %%es" : : "r"(KERNEL_DS));
	__asm__ __volatile__("movl %0, %%fs" : : "r"(KERNEL_DS));
	__asm__ __volatile__("movl %0, %%gs" : : "r"(KERNEL_DS));

	// Interruption acknowledgement
	outb(0x20, 0x20);

	switch (syscall_num)
	{
	case START:
		return start((char *)param1, (unsigned long)param2, (int)param3, (void *)param4);
		break;

	case GETPID:
		return getpid();
		break;

	case GETPRIO:
		return getprio((int)param1);
		break;

	case CHPRIO:
		return chprio((int)param1, (int)param2);
		break;

	case KILL:
		return kill((int)param1);
		break;

	case WAITPID:
		return waitpid((int)param1, (int *)param2);
		break;

	case EXIT:
		exit((int)param1);
		return 0;
		break;

	case CONS_WRITE:
		if (!valid_user_address(param1)) return 0;
		console_putbytes((char *)param1, (unsigned long)param2);
		return 0;
		break;

	case CONS_READ:
		#ifdef CONS_READ_LINE
		return console_readline((char *)param1, (unsigned long)param2);
		#elif defined CONS_READ_CHAR
		return console_readchar();
		#else
		#error "CONS_READ_LINE" ou "CONS_READ_CHAR" doit être définie.
		#endif
		return -1;
		break;

	case CONS_ECHO:
		console_echo((int)param1);
		return 0;
		break;

	case SCOUNT:
		return scount((int)param1);
		break;

	case SCREATE:
		return screate((short)(int)param1);
		break;

	case SDELETE:
		return sdelete((int)param1);
		break;

	case SIGNAL:
		return signal((int)param1);
		break;

	case SIGNALN:
		return signaln((int)param1, (short)(int)param2);
		break;

	case SRESET:
		return sreset((int)param1, (short)(int)param2);
		break;

	case TRY_WAIT:
		return try_wait((int)param1);
		break;

	case WAIT:
		return wait((int)param1);
		break;

	case PCOUNT:
		return pcount((int)param1, (int *)param2);
		break;

	case PCREATE:
		return pcreate((int)param1);
		break;

	case PDELETE:
		return pdelete((int)param1);
		break;

	case PRECEIVE:
		return preceive((int)param1, (int *)param2);
		break;

	case PRESET:
		return preset((int)param1);
		break;

	case PSEND:
		return psend((int)param1, (int)param2);
		break;

	case CLOCK_SETTINGS:
		clock_settings((unsigned long *)param1, (unsigned long *)param2);
		return 0;
		break;

	case CURRENT_CLOCK:
		return current_clock();
		break;

	case WAIT_CLOCK:
		wait_clock((unsigned long)param1);
		return 0;
		break;

	case SYS_INFO:
		sys_info();
		return 0;
		break;

	case SHM_CREATE:
		return (int)(shm_create((char *)param1));
		break;

	case SHM_ACQUIRE:
		return (int)(shm_acquire((char *)param1));
		break;

	case SHM_RELEASE:
		shm_release((char *)param1);
		return 0;
		break;

	default:
		panic("Syscall number %d not recognized !", syscall_num);
		return -1;
		break;
	}

	// Code should never reach here
	panic("Code should never reach here !");
	return -1;
}

int valid_user_address(void *address)
{
	if (address && (unsigned)address < (unsigned)0x4000000)
    {
        return 0;
    }
	return 1;
}