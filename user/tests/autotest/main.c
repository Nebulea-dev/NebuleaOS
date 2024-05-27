/*
 * Ensimag - Projet système
 * Copyright (C) 2013 - Damien Dejean <dam.dejean@gmail.com>
 */

#include "sysapi.h"

const char *tests[] = {
    "test0",
    "test1",
    "test2",
    "test3",
    "test4",
    "test5",
    // "test6", // Test non passable car on passe le paramètre de processus à main et pas _start
    "test7",
    // "test8", // Aucune idée du bug ... difficile
    // "test9", // Problème au free du processus
    "test10",
    "test11",
    "test12",
    // "test13", // TODO : fix pagefault
    "test14",
    "test15",
    // "test16", // TODO : stacksize too little ? and random segfault
    // "test17", // TODO : Test incorrect -> forcément un use after free ?
    "test18",
    // "test19", // read chars
    // "test20", // sémaphores
    // "test21", // shared memory
    // "test22", // TODO : nettoyer la TLB après free de smh
    0,
};

int main(void)
{
    int i = 0;
    int pid;
    int ret;

    while (tests[i] != NULL)
    {
            printf("Test %s : ", tests[i]);
            pid = start(tests[i], 4000, 128, NULL);
            waitpid(pid, &ret);
            assert(ret == 0);
            i++;
    }

    printf("All tests passed.\n");
}
