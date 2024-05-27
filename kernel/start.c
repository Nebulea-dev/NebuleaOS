#include "cpu.h"
#include "start.h"

#include "screen.h"
#include "time.h"
#include "process.h"
#include "info.h"
#include "message_queue.h"
#include "mmu.h"
#include "userspace_apps.h"
#include "usermode.h"
#include "segment.h"
#include "processor_structs.h"
#include "consts.h"
#include "usermode.h"
#include "syscall_kernel.h"
#include "hash.h"
#include "shared_memory.h"
#include "keyboard.h"

extern struct x86_tss tss;

void kernel_start(void)
{
    init_free_list();
    clear_screen();
    set_cursor(current_lig, current_col);
    write_info_titles();

    // Timer interrupt
    init_IT_handler(32, IT_32_handler, false);

    // Syscall interrupt
    init_IT_handler(49, IT_49_handler, true);

    // Keyboard interrupt
    init_IT_handler(33, IT_33_handler, true);

    // General Protection Fault handler
    init_IT_handler(13, IT_13_handler, false);

    // Page Fault handler
    init_IT_handler(14, IT_14_handler, false);

    set_clock_frequency(200);

    // Setup hashmap head
    initalize_hash_head();

    Process *p_idle = __init_idle();
    start("autotest", 4096, 128, NULL);

    // Starting idle process then unmasking clock interruptions
    current_pid = 0;

    // Setup kernel stack as if it was interrupted
    tss.esp0 = (int)p_idle->kernel_stack + p_idle->kernel_stack_size;

    // Unmask IRQ 1 and 2
    mask_IRQ(0, 0);
    mask_IRQ(1, 0);

    new_process_start_function();

    while (1)
    {
        panic("Kernel panic: idle process returned\n");
        hlt();
    }
}
