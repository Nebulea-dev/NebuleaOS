#include "mmu.h"
#include "stddef.h"
#include "stdio.h"
#include "debug.h"
#include "stdbool.h"
#include "consts.h"
#include "stddef.h"

int *PHYSICAL_MEMORY_HEAD = (MEMORY_START);
int *next_virtual_address = (int *)THREE_GIGABYTES;

/* TODO : les permissions sont un BORDEL sans nom.
La raison a ce bordel est la suivante : quel type de protection mettre où ? actuellement, pour des raisons de facilité
et de flemme, tout est RW pour USER + KERNEL. Sauf que dans la vie ca marche pas mdr, faut changer ca.

De plus, pour qu'une page soit RW USER, il faut que sa pgdir ET sa ptable ET sa page soit RW USER : bordel horrible
Idée : écrire une fonction set_page_flags_from_address ?*/

/**
 * create a free liste with every element pointing to the 4ko further from 1GO to 4GO
 */
void init_free_list()
{
    int *address;
    for (address = MEMORY_START; address + 4096 < MEMORY_END; address += 4096)
    {
        *address = (int)(address + 4096);
    }
    *address = (int)NULL;
}

void free_physical_memory_address(void *address)
{
    *((int *)address) = (int)PHYSICAL_MEMORY_HEAD;
    PHYSICAL_MEMORY_HEAD = address;
}

void *allocate_physical_memory_address()
{
    void *allocated_address = PHYSICAL_MEMORY_HEAD;
    PHYSICAL_MEMORY_HEAD = (void *)*((int *)PHYSICAL_MEMORY_HEAD);

    return allocated_address;
}

void map_page_from_virtual_address(unsigned *pdir,
                                   unsigned virtual_address,
                                   unsigned flags)
{
    unsigned *ptable_entry = get_ptable_entry_from_virtual_address((void *)virtual_address, pdir, true, flags);

    *ptable_entry = ((int)PHYSICAL_MEMORY_HEAD & 0xFFFFF000) | flags;

    int *next_physical_memory_head = (int *)*PHYSICAL_MEMORY_HEAD;
    PHYSICAL_MEMORY_HEAD = next_physical_memory_head;
}

void map_region(unsigned *pdir,
                unsigned initial_virtual_address,
                unsigned flags,
                unsigned size)
{
    for (unsigned total_page_size = 0; total_page_size <= size; total_page_size += 4096)
    {
        map_page_from_virtual_address(pdir, initial_virtual_address + total_page_size, flags);
    }
}

void fill_new_pgdir(unsigned pagedir[],
                    unsigned source_pagedir[])
{
    unsigned i;

    for (i = 0; i < 64; i++)
    {
        pagedir[i] = source_pagedir[i];
    }
    for (i = 64; i < 1024; i++)
    {
        pagedir[i] = 0;
    }
}

void *virtual_malloc(int size)
{
        // void *initial_virtual_address = next_virtual_address;
    void *initial_virtual_address = NULL;
    void *allocated_address;
    void *virtual_allocated_address;

    for (int nb_of_pages_allocated = 0; nb_of_pages_allocated * 4096 < size; nb_of_pages_allocated++)
    {
                allocated_address = allocate_physical_memory_address();
                if (allocated_address == NULL)
        {
                        return NULL;
        }

        virtual_allocated_address = allocated_address;
        if (initial_virtual_address == NULL)
        {
            initial_virtual_address = allocated_address;
        }

        unsigned *ptable_entry = get_ptable_entry_from_virtual_address(virtual_allocated_address, get_current_pagedir(), true, PAGE_TABLE_USER_RW);

        *ptable_entry = ((int)allocated_address & 0xFFFFF000) | PAGE_TABLE_USER_RW;
    }

    return initial_virtual_address;
}

void virtual_free(void *address, int size)
{
    void *physical_address;
    unsigned *ptable_entry;

    for (int nb_of_pages_allocated = 0; nb_of_pages_allocated * 4096 < size; nb_of_pages_allocated++)
    {
        ptable_entry = get_ptable_entry_from_virtual_address(address, get_current_pagedir(), false, 0);
        physical_address = (void *)(*ptable_entry & 0xFFFFF000);

        free_physical_memory_address(physical_address);

        *ptable_entry = ((int)physical_address & 0xFFFFF000) | PAGE_TABLE_RW;

        address = (void *)((int)address + 4096);
    }
}

void pgdir_copy(unsigned *source_pgdir, unsigned *dest_pgdir, void *source_address, void *dest_address, int size)
{
    for (int i = 0; i * 4096 < size; i++)
    {
        unsigned *source_ptable_entry = get_ptable_entry_from_virtual_address(source_address + 4096 * i, source_pgdir, false, 0);
        unsigned *dest_ptable_entry = get_ptable_entry_from_virtual_address(dest_address + 4096 * i, dest_pgdir, true, PAGE_TABLE_USER_RW);

        // La ligne suivante a réglé un bug critique, mais c'est comme mettre un pansement sur quelqu'un qui a perdu une jambe
        // si il y a un problème avec la MMU c'est probablement ici
        *dest_ptable_entry = (*source_ptable_entry & 0xFFFFF000) | PAGE_TABLE_USER_RW;
    }
}

unsigned *get_ptable_entry_from_virtual_address(void *virtual_address, unsigned *pgdir, bool create_if_not_present, unsigned flags)
{
    unsigned pd_index = (unsigned)virtual_address >> 22;
    unsigned pt_index = ((unsigned)virtual_address >> 12) & 0x3FFu;

    /* Get page table */
    unsigned *ptable = (unsigned *)(pgdir[pd_index] & 0xFFFFF000);
    if (!ptable)
    {
        if (!create_if_not_present)
        {
            panic("Failed to get page table in get_ptable_entry_from_virtual_address\n");
            return NULL;
        }

        /* Allocate page table */
        ptable = (unsigned *)allocate_physical_memory_address();
        if (!ptable)
        {
            panic("Failed to allocate page table in get_ptable_entry_from_virtual_address\n");
            return NULL;
        }

        /* Clear page table */
        for (unsigned i = 0; i < 1024; i++)
        {
            ptable[i] = 0;
        }

        /* Set page table entry in page directory */
        pgdir[pd_index] = ((unsigned)ptable & 0xFFFFF000) | flags;
    }

    if(create_if_not_present && flags)
    {
        set_table_permission(virtual_address, pgdir, flags);
    }

    return (unsigned *)(ptable + pt_index);
}

unsigned *get_physical_address_from_virtual_address(void *virtual_address, unsigned *pgdir)
{
    unsigned *ptable_entry = get_ptable_entry_from_virtual_address(virtual_address, pgdir, false, 0);
    unsigned physical_address = (*ptable_entry & 0xFFFFF000) + ((unsigned)virtual_address & 0xFFF);
    return (unsigned *)physical_address;
}

void map_region_to_other_region(void *source_address, void *dest_address, int nb_of_pages)
{
    unsigned *source_ptable_entry;
    unsigned *dest_ptable_entry;

    for (int i = 0; i < nb_of_pages; i++)
    {
        source_ptable_entry = get_ptable_entry_from_virtual_address(source_address, get_current_pagedir(), false, 0);
        dest_ptable_entry = get_ptable_entry_from_virtual_address(dest_address, get_current_pagedir(), true, PAGE_TABLE_USER_RW);

        *dest_ptable_entry = *source_ptable_entry;

        source_address = (void *)((int)source_address + 4096);
        dest_address = (void *)((int)dest_address + 4096);
    }
}

unsigned *get_current_pagedir()
{
    unsigned *pgdir;
    __asm__ __volatile__("movl %%cr3, %0" : "=r"(pgdir));
    return pgdir;
}

void set_address_permission(void *address, unsigned *pgdir, unsigned flags)
{
    unsigned *ptable_entry = get_ptable_entry_from_virtual_address(address, pgdir, true, PAGE_TABLE_USER_RW);
    *ptable_entry = (*ptable_entry & 0xFFFFF000) | flags;
    printf("Setting permission of address %p to %x\n", address, flags);
}

void set_table_permission(void *address, unsigned *pgdir, unsigned flags)
{
    unsigned pd_index = (unsigned)address >> 22;

    /* Get page table */
    unsigned *pdirectory = (unsigned *)(pgdir + pd_index);
    *pdirectory = (*pdirectory & 0xFFFFF000) | flags;
}

void get_permission_of_page(unsigned *pgdir, void *virtual_address)
{
    
    unsigned pd_index = (unsigned)virtual_address >> 22;
    unsigned pt_index = ((unsigned)virtual_address >> 12) & 0x3FFu;

    unsigned *ptable = (unsigned*)pgdir[pd_index];
    unsigned ptable_entry = (unsigned)ptable + pt_index;

    unsigned flags = (unsigned)ptable_entry & 0b111;

    switch (flags)
    {
    case PAGE_TABLE_USER_RW:
        printf("Page %p is USER RW\n", virtual_address);
        break;
    case PAGE_TABLE_USER_RO:
        printf("Page %p is USER RO\n", virtual_address);
        break;
    case PAGE_TABLE_RW:
        printf("Page %p is KERNEL RW\n", virtual_address);
        break;
    case PAGE_TABLE_RO:
        printf("Page %p is KERNEL RO\n", virtual_address);
        break;
    default:
        printf("Page %p has unknown permissions\n", virtual_address);
        break;
    }
}

unsigned *allocate_virtual_memory_address()
{
    unsigned *allocated_address = (unsigned *)next_virtual_address;
    next_virtual_address += 4096;
    return allocated_address;
}