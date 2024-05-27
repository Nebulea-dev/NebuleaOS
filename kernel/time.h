#include "stdbool.h"
#include "stdint.h"

#define SCHEDFREQ 5
#define QUARTZ 0x1234DD

extern unsigned long quartz;
extern unsigned long frequency;

extern uint32_t time_count;
extern uint8_t h;
extern uint8_t m;
extern uint8_t s;

extern void IT_32_handler(void);
extern void IT_49_handler(void);
extern void IT_13_handler(void);
extern void IT_14_handler(void);

void write_time(char *s, int len);
void init_IT_handler(uint32_t num_IT, void (*handler)(void), bool software_interrupt);
void tic_PIT(void);
void mask_IRQ(uint32_t num_IRQ, bool mask);
void set_clock_frequency(unsigned long frequency);

// public API
void clock_settings(unsigned long *quartz, unsigned long *ticks);
unsigned long current_clock(void);
void wait_clock(unsigned long wakeup);
