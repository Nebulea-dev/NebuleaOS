#ifndef PSYS_BASE_SEMAPHORE_H
#define PSYS_BASE_SEMAPHORE_H

#include "stddef.h"
#include "queue.h"
#include "stdbool.h"

#define NBSEMAPHORE 10000
#define SEMAPHORE_MAX_SIZE 32767

/**
 * A MessageQueue is implemented by :
 * - A queue of process blocked waiting (either to send or to receive a message)
 * - A circular buffer of messages
 */
typedef struct Semaphore
{
    int sid;
    short int count;
    link waiting_process;
    bool was_reset;
    bool was_deleted;
} Semaphore;

extern Semaphore *semaphore_table[NBSEMAPHORE];

int __get_free_sem(void);
int __nb_waiting_proc2(int sem);

// Public API
int screate(short int count);
int sdelete(int sem);
int wait(int sem);
int try_wait(int sem);
int signal(int sem);
int signaln(int sem, short int count);
int scount(int sem);
int sreset(int sem, short int count);

#endif // PSYS_BASE_SEMAPHORE_H
