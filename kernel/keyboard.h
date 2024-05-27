#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "kbd.h"
#include "debug.h"
#include "cpu.h"
#include "stdint.h"

extern void IT_33_handler(void);
extern uint8_t CONSOLE_ECHO;

void keyboard_interrupt(void);

void console_echo(int on);
int console_readchar(void);
int console_readline(char *str, unsigned long len);

#endif
