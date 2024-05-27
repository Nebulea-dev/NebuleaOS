/*
 * Ensimag - Projet système
 * Copyright (C) 2012 - Damien Dejean <dam.dejean@gmail.com>
 *
 * Stub for console_putbytes system call.
 */

extern void cons_write(const char *str, unsigned long size);

void console_putbytes(const char *s, int len)
{
        cons_write(s, len);
}