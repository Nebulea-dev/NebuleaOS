#include "cpu.h"
#include "string.h"

#include "screen.h"
#include "info.h"
#include "keyboard.h"

uint8_t current_lig = INFO_SIZE;
uint8_t current_col = 0;
uint8_t background_color = 0;
uint8_t foreground_color = 15;

// Function to get the memory address of a specific location on the screen
uint16_t *ptr_mem(uint8_t lig, uint8_t col)
{
    return (uint16_t *)(0xB8000 + 2 * (lig * NB_COLUMNS + col));
}

// Function to write a character at a specific location on the screen
void write_car(uint8_t lig, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    uint16_t *address = ptr_mem(lig, col);
    uint16_t word = (0 << 15) | (bg << 12) | (fg << 8) | c;
    if (lig >= NB_LINES || col >= NB_COLUMNS)
        return;
    *address = word;
}

// Function to clear the screen
void clear_screen()
{
    for (int i = 0; i < NB_LINES; i++)
    {
        for (int j = 0; j < NB_COLUMNS; j++)
        {
            write_car(i, j, 32, 15, 0);
        }
    }
}

// Function to place the cursor at a specific location on the screen
void set_cursor(uint8_t lig, uint8_t col)
{
    if (lig >= NB_LINES || col >= NB_COLUMNS)
        return;
    uint16_t pos = col + lig * NB_COLUMNS;
    uint8_t little = (uint8_t)(pos & 0xFF);
    uint8_t big = (uint8_t)((pos & 0xFF00) >> 8);

    outb(0x0F, CURSOR_COM_PORT);
    outb(little, CURSOR_DATA_PORT);
    outb(0x0E, CURSOR_COM_PORT);
    outb(big, CURSOR_DATA_PORT);

    current_lig = lig;
    current_col = col;
}

// Function to handle a character input
void handle_car(char c)
{
    switch (c)
    {
    case 8:
        // Handle backspace
        if (current_col != 0)
        {
            current_col--;
        }
        set_cursor(current_lig, current_col);
        break;
    case 9:
        // Handle tab
        current_col = current_col - current_col % 4 + 4;
        if (current_col >= NB_COLUMNS)
            current_col = NB_COLUMNS - 1;
        set_cursor(current_lig, current_col);
        break;
    case 10:
        // Handle newline
        current_col = 0;
        if (current_lig < NB_LINES - 1)
        {
            current_lig++;
        }
        else
        {
            scroll();
        }
        set_cursor(current_lig, current_col);
        break;
    case 12:
        // Handle form feed
        current_col = 0;
        current_lig = 0;
        clear_screen();
        set_cursor(current_lig, current_col);
        break;
    case 13:
        // Handle carriage return
        current_col = 0;
        set_cursor(current_lig, current_col);
        break;
    default:
        // Handle printable characters
        if (c > 31 && c < 127)
        {
            write_car(current_lig, current_col, c, foreground_color, background_color);

            uint8_t new_col = current_col;
            uint8_t new_lig = current_lig;
            if (current_col == NB_COLUMNS - 1 && current_lig != NB_LINES - 1)
            {
                new_col = 0;
                new_lig += 1;
            }
            else if (current_col == NB_COLUMNS - 1 && current_lig == NB_LINES - 1)
            {
                new_col = 0;
                scroll();
            }
            else
            {
                new_col += 1;
            }

            current_lig = new_lig;
            current_col = new_col;
            set_cursor(current_lig, current_col);
        }
        break;
    }
}

// Function to scroll the screen up by one line
void scroll()
{
    memmove((void *)STARTING_ADDRESS + NB_COLUMNS * 2 * INFO_SIZE,
            (void *)(STARTING_ADDRESS + NB_COLUMNS * 2 * (INFO_SIZE + 1)),
            NB_COLUMNS * (NB_LINES - INFO_SIZE - 1) * 2 * 8);
    for (int i = 0; i < NB_COLUMNS; i++)
    {
        write_car(NB_LINES - 1, i, 32, 15, 0);
    }
}

// Function to print a string of characters on the screen
void console_putbytes(const char *s, int len)
{
    for (int i = 0; i < len; i++)
    {
        handle_car(s[i]);
    }
}