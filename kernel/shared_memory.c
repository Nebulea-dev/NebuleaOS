#include "shared_memory.h"
#include "hash.h"
#include "mmu.h"
#include "mem.h"
#include "stdio.h"
#include "string.h"

int NUMBER_SHARED_PAGES = 0;
hash_t *hash_head = NULL;

#define MAX_SHARED_PAGES 100

void initalize_hash_head()
{
	hash_head = mem_alloc(sizeof(hash_t));
	hash_init_string(hash_head);
}

void *shm_create(const char *key)
{
	// Check if we have reached the maximum number of shared pages
	if (NUMBER_SHARED_PAGES >= MAX_SHARED_PAGES)
	{
		return NULL;
	};
	NUMBER_SHARED_PAGES++;

	// Check if key is NULL or already exists
	if (key == NULL)
	{
		return NULL;
	};
	if (hash_get(hash_head, (char *)key, NULL) != NULL)
	{
		return NULL;
	};

	// Create shared memory
	void *shared_memory = virtual_malloc(4096);

	// we NEED to have the key address allocated in the kernel space because the hash table will store the address of the key
	// and will not strcpy it. Which means when changing process, the key may be invalid.
	// Since the kernel space is 1 to 1 mapped, we are sure that this address never changes.
	char *key_address = mem_alloc(strlen(key) + 1);
	strcpy(key_address, key);

	hash_set(hash_head, key_address, shared_memory);
	return shared_memory;
}

void *shm_acquire(const char *key)
{
	// Get shared memory address
	void *shared_memory = hash_get(hash_head, (char *)key, NULL);
	if (shared_memory == NULL)
	{
		printf("Shared memory for key %s not found\n", key);
	};

	// Map shared memory to the current process
	unsigned *ptable_entry = get_ptable_entry_from_virtual_address(shared_memory, get_current_pagedir(), true, PAGE_TABLE_USER_RW);
	*ptable_entry = ((int)shared_memory & 0xFFFFF000) | PAGE_TABLE_USER_RW;

	return shared_memory;
}

void shm_release(const char *key)
{
	if (NUMBER_SHARED_PAGES < 0)
	{
		return;
	};
	NUMBER_SHARED_PAGES--;

	void *address = hash_get(hash_head, (char *)key, NULL);

	if (address != NULL)
	{
		virtual_free(address, 4096);
		hash_del(hash_head, (char *)key);
	}
}