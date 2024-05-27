#include "semaphore.h"

#include "queue.h"
#include "stdbool.h"

#include "process.h"

// Id of a semaphore is its index in the table
Semaphore *semaphore_table[NBSEMAPHORE];

int screate(short int count)
{
    int sid = __get_free_sem();
    if (count < 0 || sid == -1)
    {
        return -1;
    }
    Semaphore *sem = mem_alloc(sizeof(Semaphore));
    semaphore_table[sid] = sem;
    sem->sid = sid;
    sem->count = count;
    sem->was_reset = false;
    INIT_LIST_HEAD(&sem->waiting_process);

    return sid;
}

int sdelete(int sem)
{
    if (sem < 0 || sem > NBSEMAPHORE - 1 || !semaphore_table[sem])
    {
        return -1;
    }

    // All blocked process become activable again
    Process *current;
    while (!queue_empty(&semaphore_table[sem]->waiting_process))
    {
        current = queue_out(&semaphore_table[sem]->waiting_process, Process, link);
        current->state = ACTIVABLE;
        queue_add(current, &activable_list, Process, link, priority);
    }
    Semaphore *deleted = semaphore_table[sem];
    semaphore_table[sem] = NULL;
    if (__nb_waiting_proc2(sem) == 0) {
        mem_free(deleted, sizeof(Semaphore));
    } else {
        deleted->was_deleted = true;
    }

    process_table[current_pid]->state = ACTIVABLE;
    queue_add(process_table[current_pid], &activable_list, Process, link, priority);
    __scheduler();

    return 0;
}

int signal(int sem)
{
    if (sem < 0 || sem > NBSEMAPHORE - 1 || !semaphore_table[sem])
    {
        return -1;
    }

    Semaphore *s = semaphore_table[sem];
    if (s->count >= SEMAPHORE_MAX_SIZE)
    {
        return -2;
    }

    s->count++;
    if (s->count <= 0)
    {

        semaphore_table[sem]->was_reset = false;

        Process *p = queue_out(&s->waiting_process, Process, link);
        p->state = ACTIVABLE;
        queue_add(p, &activable_list, Process, link, priority);

        process_table[current_pid]->state = ACTIVABLE;
        queue_add(process_table[current_pid], &activable_list, Process, link, priority);
        __scheduler();
    }

    return 0;
}

int signaln(int sem, short int count)
{
    if (sem < 0 || sem > NBSEMAPHORE - 1 || !semaphore_table[sem])
    {
        return -1;
    }

    if (count < 0)
    {
        return -2;
    }

    Semaphore *s = semaphore_table[sem];
    if (s->count + count > SEMAPHORE_MAX_SIZE)
    {
        return -2;
    }

    semaphore_table[sem]->was_reset = false;
    for (int i = 0; i < count; i++)
    {
        s->count++;
        if (s->count <= 0)
        {
            Process *p = queue_out(&s->waiting_process, Process, link);
            p->state = ACTIVABLE;
            queue_add(p, &activable_list, Process, link, priority);
        }
    }
    process_table[current_pid]->state = ACTIVABLE;
    queue_add(process_table[current_pid], &activable_list, Process, link, priority);
    __scheduler();

    return 0;
}

int try_wait(int sem)
{
    if (sem < 0 || sem > NBSEMAPHORE - 1 || !semaphore_table[sem])
    {
        return -1;
    }

    Semaphore *s = semaphore_table[sem];
    if (s->count == -SEMAPHORE_MAX_SIZE)
    {
        return -2;
    }

    if (s->count > 0)
    {
        s->count--;
        return 0;
    }
    else
    {
        return -3;
    }
}

int wait(int sem)
{
    if (sem < 0 || sem > NBSEMAPHORE - 1 || !semaphore_table[sem])
    {
        return -1;
    }

    Semaphore *s = semaphore_table[sem];
    if (s->count <= -SEMAPHORE_MAX_SIZE)
    {
        return -2;
    }

    s->count--;
    if (s->count < 0)
    {

        process_table[current_pid]->state = IN_SEM;
        process_table[current_pid]->waited_sem = sem;
        queue_add(process_table[current_pid], &s->waiting_process, Process, link, priority);

        __scheduler();

        if (s->was_deleted)
        {
            mem_free(s, sizeof(Semaphore));
            return -3;
        }

        if (s->was_reset)
        {
            return -4;
        }
    }

    return 0;
}

int sreset(int sem, short int count)
{
    if (sem < 0 || sem > NBSEMAPHORE - 1 || !semaphore_table[sem])
    {
        return -1;
    }

    if (count < 0)
    {
        return -1;
    }

    Process *current;
    while (!queue_empty(&semaphore_table[sem]->waiting_process))
    {
        current = queue_out(&semaphore_table[sem]->waiting_process, Process, link);
        current->state = ACTIVABLE;
        queue_add(current, &activable_list, Process, link, priority);
    }

    semaphore_table[sem]->count = count;
    semaphore_table[sem]->was_reset = true;

    process_table[current_pid]->state = ACTIVABLE;
    queue_add(process_table[current_pid], &activable_list, Process, link, priority);
    __scheduler();

    return 0;
}

int scount(int sem)
{
    if (sem < 0 || sem > NBSEMAPHORE - 1 || !semaphore_table[sem])
    {
        return -1;
    }

    Semaphore *s = semaphore_table[sem];
    return ((int)s->count & 0xffff);
}

int __nb_waiting_proc2(int sem)
{
    int n = 0;
    Process *current;
    queue_for_each(current, &semaphore_table[sem]->waiting_process, Process, link)
    {
        n++;
    }
    return n;
}

int __get_free_sem()
{
    int sem = -1;
    for (int i = 0; i < NBSEMAPHORE; i++)
    {
        if (semaphore_table[i] == NULL)
        {
            sem = i;
            break;
        }
    }
    return sem;
}