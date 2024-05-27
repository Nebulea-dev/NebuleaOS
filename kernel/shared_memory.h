#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "stddef.h"
#include "hash.h"

extern hash_t * hash_head;	

void initalize_hash_head();
void *shm_create(const char *);
void *shm_acquire(const char *);
void shm_release(const char *);

#endif