#include "stdarg.h"
#include "syscall_user.h"

#define USER_DS 0x4b
#define WITH_MSG 1
#define CONS_READ_CHAR 1

int start(const char *process_name, unsigned long ssize, int prio, void *arg)
{
	return __syscall(START, 4, process_name, ssize, prio, arg);
}

int getpid(void)
{
	return __syscall(GETPID, 0);
}

int getprio(int pid)
{
	return __syscall(GETPRIO, 1, pid);
}

int chprio(int pid, int newprio)
{
	return __syscall(CHPRIO, 2, pid, newprio);
}

int kill(int pid)
{
	return __syscall(KILL, 1, pid);
}

int waitpid(int pid, int *retval)
{
	return __syscall(WAITPID, 2, pid, retval);
}

void exit(int retval)
{
	__syscall(EXIT, 1, retval);

	// We add this loop to indicate to the compiler that the function will never return
	while (1)
		;
}

void cons_write(const char *str, unsigned long size)
{
	__syscall(CONS_WRITE, 2, str, size);
}

#if defined CONS_READ_LINE
unsigned long cons_read(char *string, unsigned long length)
{
	return __syscall(CONS_READ, 2, string, length);
}

#elif defined CONS_READ_CHAR
int cons_read(void)
{
	return __syscall(CONS_READ, 0);
}

#endif

void cons_echo(int on)
{
	__syscall(CONS_ECHO, 1, on);
}

#if defined WITH_SEM

int scount(int sem)
{
	return __syscall(SCOUNT, 1, sem);
}

int screate(short count)
{
	return __syscall(SCREATE, 1, count);
}

int sdelete(int sem)
{
	return __syscall(SDELETE, 1, sem);
}

int signal(int sem)
{
	return __syscall(SIGNAL, 1, sem);
}

int signaln(int sem, short count)
{
	return __syscall(SIGNALN, 2, sem, count);
}

int sreset(int sem, short count)
{
	return __syscall(SRESET, 2, sem, count);
}

int try_wait(int sem)
{
	return __syscall(TRY_WAIT, 1, sem);
}

int wait(int sem)
{
	return __syscall(WAIT, 1, sem);
}

#elif defined WITH_MSG

int pcount(int fid, int *count)
{
	return __syscall(PCOUNT, 2, fid, count);
}

int pcreate(int count)
{
	return __syscall(PCREATE, 1, count);
}

int pdelete(int fid)
{
	return __syscall(PDELETE, 1, fid);
}

int preceive(int fid, int *message)
{
	return __syscall(PRECEIVE, 2, fid, message);
}

int preset(int fid)
{
	return __syscall(PRESET, 1, fid);
}

int psend(int fid, int message)
{
	return __syscall(PSEND, 2, fid, message);
}

#endif

void clock_settings(unsigned long *quartz, unsigned long *ticks)
{
	__syscall(CLOCK_SETTINGS, 2, quartz, ticks);
}

unsigned long current_clock(void)
{
	return __syscall(CURRENT_CLOCK, 0);
}

void wait_clock(unsigned long wakeup)
{
	__syscall(WAIT_CLOCK, 1, wakeup);
}

void sys_info(void)
{
	__syscall(SYS_INFO, 0);
}

int shm_create(const char *key)
{
	return __syscall(SHM_CREATE, 1, key);
}

int shm_acquire(const char *key)
{
	return __syscall(SHM_ACQUIRE, 1, key);
}

int shm_release(const char *key)
{
	return __syscall(SHM_RELEASE, 1, key);
}

int __syscall(enum syscall_number num, int args_number, ...)
{
	int return_value;
	void *args[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

	va_list ap;
	va_start(ap, args_number);

	for (int i = 0; i < args_number; i++)
	{
		args[i] = va_arg(ap, void *);
	}

	va_end(ap);

	__do_interruption(num, args);

	// Return value of __do_interruption should be in EAX
	__asm__ __volatile__("movl %%eax, %0" : "=r"(return_value));

	__asm__ __volatile__("movl %0, %%ds" : : "r"(USER_DS));
	__asm__ __volatile__("movl %0, %%es" : : "r"(USER_DS));
	__asm__ __volatile__("movl %0, %%fs" : : "r"(USER_DS));
	__asm__ __volatile__("movl %0, %%gs" : : "r"(USER_DS));

	return return_value;
}