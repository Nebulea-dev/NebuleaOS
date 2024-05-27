#include "cpu.h"
#include "segment.h"

#include "time.h"
#include "screen.h"
#include "process.h"
#include "info.h"

unsigned long clock_ticks = 200;
unsigned long clock_frequency = 0;

uint32_t time_count = 0;
uint8_t h = 0;
uint8_t m = 0;
uint8_t s = 0;

void write_time(char *s, int len)
{
    if (len <= 80)
    {
        for (int i = 0; i < len - 1; i++)
        {
            write_car(0, 79 - i, s[(len - 2) - i], foreground_color, background_color);
        }
    }
}

void set_clock_frequency(unsigned long frequency)
{
    clock_frequency = frequency;
    clock_ticks = QUARTZ/frequency;

    uint8_t poids_faible = (clock_ticks & 0xFF);
    uint8_t poids_fort = (clock_ticks & 0xFF00) >> 8;

    outb(0x34, 0x43);
    outb(poids_faible, 0x40);
    outb(poids_fort, 0x40);
}

void clock_settings(unsigned long *quartz, unsigned long *ticks)
{
    *quartz = QUARTZ;
    *ticks = clock_ticks;
}

void init_IT_handler(uint32_t num_IT, void (*handler)(void), bool software_interrupt)
{
    uint32_t *start = (uint32_t *)(0x1000 + 8 * num_IT);
    uint16_t gate = software_interrupt ? 0xEE00 : 0x8E00;

    *start = (KERNEL_CS << 16) | ((uint32_t)(handler) & 0xFFFF);
    *(start + 1) = ((uint32_t)(handler) & 0xFFFF0000) | gate;
}

void tic_PIT()
{
    __asm__ __volatile__("movl %0, %%ds" : : "r"(KERNEL_DS));
    __asm__ __volatile__("movl %0, %%es" : : "r"(KERNEL_DS));
    __asm__ __volatile__("movl %0, %%fs" : : "r"(KERNEL_DS));
    __asm__ __volatile__("movl %0, %%gs" : : "r"(KERNEL_DS));

    // Interruption acknowledgement
    outb(0x20, 0x20);

    // Printing spent time
    time_count++;
    if (time_count % clock_frequency == 0)
    {
        h = (time_count / clock_frequency) / 3600;
        m = ((time_count / clock_frequency) % 3600) / 60;
        s = ((time_count / clock_frequency) % 3600) % 60;
    }
    char string[64];
    sprintf(string, "%d%d:%d%d:%d%d", h / 10, h % 10, m / 10, m % 10, s / 10,
            s % 10);
    write_time(string, 10);

    // Printing process information
    write_process_table();
    write_queue_table();
    write_activable();
    write_finished();

    // Process scheduling
    if (time_count % SCHEDFREQ == 0)
    {
        Process *old_process = process_table[current_pid];
        if (old_process->state == ELECTED)
        {
            old_process->state = ACTIVABLE;
            queue_add(old_process, &activable_list, Process, link, priority);
        }

        __scheduler();
    }

    __asm__ __volatile__("movl %0, %%ds" : : "r"(USER_DS));
    __asm__ __volatile__("movl %0, %%es" : : "r"(USER_DS));
    __asm__ __volatile__("movl %0, %%fs" : : "r"(USER_DS));
    __asm__ __volatile__("movl %0, %%gs" : : "r"(USER_DS));
}

void mask_IRQ(uint32_t num_IRQ, bool mask)
{
    uint8_t bool_tab = inb(0x21);
    if (!mask)
    {
        bool_tab &= (uint8_t) ~(1 << num_IRQ);
    }
    else
    {
        bool_tab |= (uint8_t)1 << num_IRQ;
    }
    outb(bool_tab, 0x21);
}

unsigned long current_clock(void)
{
    return time_count;
}
