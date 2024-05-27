#include "keyboard.h"

uint8_t CONSOLE_ECHO = 1;
char readchar = 0;

void keyboard_interrupt(void)
{
	// Read the data port to acknowledge the interrupt
	uint8_t scancode = inb(0x60);	
	do_scancode(scancode);

    // Interruption acknowledgement
    outb(0x20, 0x20);
}

void keyboard_data(char *str)
{
	// Print the string only if the console echo is enabled
	if (CONSOLE_ECHO)
		printf("%s", str);
}

void console_echo(int on)
{
    CONSOLE_ECHO = on;
}

int console_readchar(void)
{
	// Wait for a character to be read
	while (readchar == 0);

	// Read the character and reset the readchar variable
	char c = readchar;
	readchar = 0;

	return c;
}

int console_readline(char *s, unsigned long len)
{
    (void)s;
    (void)len;
    // TODO: Implement console_readline phase 6
	return 0;
}