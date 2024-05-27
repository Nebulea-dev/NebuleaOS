// API for the userspace
// This file implements all the weak stubs of weak-syscall-stubs.S
#include "syscall_nums.h"

#define NULL ((void *)0)

int __syscall(enum syscall_number, int args_num, ...);
int __do_interruption(int syscall_number, void* args);