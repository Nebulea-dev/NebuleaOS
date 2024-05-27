/*
 * Ensimag - Projet système
 * Copyright (C) 2012 - Damien Dejean <dam.dejean@gmail.com>
 */

#include "sysapi.h"

int assert_failed(const char *cond, const char *file, int line)
{
        safe_printf("%s:%d: assertion '%s' failed.\n", file, line, cond);
        while (1);
        exit(-1);
        *(char *)0 = 0;
}

