#include "interruptions.h"
#include "process.h"

// Implement the interrupt handlers
void gpf_interrupt(void)
{
	panic("General Protection Fault occurred\n");
}

void pagefault_interrupt(void)
{
	printf("Page Fault occurred\n");
	exit(0);
}