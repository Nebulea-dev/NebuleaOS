#include "stdint.h"

extern uint8_t current_lig;
extern uint8_t current_col;
extern uint8_t background_color;
extern uint8_t foreground_color;

#define STARTING_ADDRESS 0xB8000
#define NB_COLUMNS 80
#define NB_LINES 25

#define CURSOR_COM_PORT 0x3D4
#define CURSOR_DATA_PORT 0x3D5

uint16_t *ptr_mem(uint8_t lig, uint8_t col);
void write_car(uint8_t lig, uint8_t col, char c, uint8_t fg, uint8_t bg);
void clear_screen();
void set_cursor(uint8_t lig, uint8_t col);
void handle_car(char c);
void scroll();
void console_putbytes(const char *s, int len);
