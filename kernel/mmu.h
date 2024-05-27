#ifndef MMU_H
#define MMU_H

#include "stdbool.h"

#define MEMORY_START (int *)(0x04000000)
#define MEMORY_END (int *)(0x10000000)

#define PAGE_TABLE_RO 0x00000001u
#define PAGE_TABLE_RW 0x00000003u
#define PAGE_TABLE_USER_RO 0x00000005u
#define PAGE_TABLE_USER_RW 0x00000007u

#define PAGE_DIR_FLAGS 0x00000003u
#define PAGE_DIR_USER_FLAGS 0x00000007u

void init_free_list();
void free_physical_memory_address(void *address);
void *allocate_physical_memory_address();
void map_page_from_virtual_address(unsigned *pdir,
								   unsigned virtual_address,
								   unsigned flags);
void map_region(unsigned *pdir,
				unsigned virtual_address,
				unsigned flags,
				unsigned size);
void fill_new_pgdir(unsigned pagedir[],
					unsigned source_pagedir[]);
void pgdir_copy(unsigned *source_pgdir, unsigned *dest_pgdir, void *source_address, void *dest_address, int size);
void *virtual_malloc(int size);
void virtual_free(void *address, int size);
unsigned *get_current_pagedir();
unsigned *get_ptable_entry_from_virtual_address(void *virtual_address, unsigned *pgdir, bool create_if_not_present, unsigned flags);

unsigned *get_physical_address_from_virtual_address(void *virtual_address, unsigned *pgdir);
void map_region_to_other_region(void *source_address, void *dest_address, int nb_of_pages);
void set_address_permission(void *address, unsigned *pgdir, unsigned flags);
void set_table_permission(void *address, unsigned *pgdir, unsigned flags);
void get_permission_of_page(unsigned *pgdir, void *virtual_address);
unsigned *allocate_virtual_memory_address();

#endif