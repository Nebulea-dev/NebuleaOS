#include "queue.h"
#include "stdbool.h"

#include "process.h"
#include "message_queue.h"

// Fid of a message queue is its index in the table
MessageQueue *queue_table[NBQUEUE];

/**
 * @brief Creating a queue.
 *
 * This function allocates a message queue i.e. a message buffer and queue of process blocked waiting.
 *
 * @param count The capacity of the buffer.
 *
 * @return -1 if count value is not valid or if the queue table is full. Else the fid of the created queue.
 */
int pcreate(int count)
{
    int fid = __get_free_fid();
    if (count <= 0 || fid == -1 || count > QUEUE_MAX_SIZE)
    {
        return -1;
    }
    MessageQueue *queue = mem_alloc(sizeof(MessageQueue));
    queue_table[fid] = queue;
    queue->fid = fid;
    queue->capacity = count;
    queue->nb_msg = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->buffer = mem_alloc(count * sizeof(int));
    INIT_LIST_HEAD(&queue->waiting_process);

    return fid;
}

/**
 * @brief Deletes a message queue.
 *
 * This function deletes a message queue by making all the stuck process activable and freeing the structures.
 *
 * @param fid Fid of the targeted process
 *
 * @return -1 if fid value is not valid. Else 0.
 */
int pdelete(int fid)
{
    if (fid < 0 || fid > NBQUEUE - 1 || !queue_table[fid])
    {
        return -1;
    }

    // All blocked process become activable again
    Process *current;
    while (!queue_empty(&queue_table[fid]->waiting_process))
    {
        current = queue_out(&queue_table[fid]->waiting_process, Process, link);
        current->state = ACTIVABLE;
        current->skip_message = true;
        queue_add(current, &activable_list, Process, link, priority);
    }
    MessageQueue *deleted = queue_table[fid];
    queue_table[fid] = NULL;
    mem_free(deleted->buffer, deleted->capacity * sizeof(int));
    mem_free(deleted, sizeof(MessageQueue));

    process_table[current_pid]->state = ACTIVABLE;
    queue_add(process_table[current_pid], &activable_list, Process, link, priority);
    __scheduler();

    return 0;
}

/**
 * @brief Sends a message to a queue.
 *
 * This function sends a message to a queue.
 * If the buffer is full, the calling process gets stuck in the queue. On the next call to preceive,
 * the least recent process in the queue will become activable and its message written in the buffer.
 * If the buffer was empty and some process are waiting to read it, instantly hands over to the least recent one.
 *
 * @param fid Fid of the targeted queue
 * @param message The message to write in the buffer
 *
 * @return -1 if count value is not valid or if the queue got deleted or reset while the calling process was stuck.
 * Else 0.
 */
int psend(int fid, int message)
{
    if (fid < 0 || fid > NBQUEUE - 1 || !queue_table[fid])
    {
        return -1;
    }

    // Saving the message in the process structure in case the buffer is full and the process starts waiting
    process_table[current_pid]->message = message;

    MessageQueue *q = queue_table[fid];
    if (q->nb_msg == 0 && !queue_empty(&q->waiting_process))
    {
        // Adding the message to the buffer
        q->buffer[q->head] = message;
        q->head = q->head + 1 > q->capacity - 1 ? 0 : q->head + 1;
        q->nb_msg++;

        // Switching to the receiver context instantly
        Process *new_process = queue_out(&q->waiting_process, Process, link);

        Process *old_process = process_table[current_pid];
        old_process->state = ACTIVABLE;
        queue_add(old_process, &activable_list, Process, link, priority);

        new_process->state = ELECTED;
        current_pid = new_process->pid;
        virtual_ctx_sw(old_process, new_process);

        return 0;
    }

    if (q->nb_msg == q->capacity)
    {
        Process *p = process_table[current_pid];
        p->state = IN_MSGQUEUE;
        queue_add(p, &q->waiting_process, Process, link, priority);

        __scheduler();

        // Code reaches here if the queue is deleted, reset
        // or if preceive was called, wrote the message of this process in the buffer and made it activable.

        // If the queue was deleted or reset, return value is negative
        if (p->skip_message)
        {
            p->skip_message = false;
            return -1;
        }

        return 0;
    }

    // Adding the message to the buffer
    q->buffer[q->head] = message;
    q->head = q->head + 1 > q->capacity - 1 ? 0 : q->head + 1;
    q->nb_msg++;

    return 0;
}

/**
 * @brief Receives a message from a queue.
 *
 * This function receives a message from a queue.
 * If the buffer is empty, the process gets stuck in the queue. The next process calling psend will hand over to the
 * least recent waiting process instanty. This process will then receive the new message.
 * If the buffer was full and some process are waiting to write, complete the buffer with the least recent one's message.
 *
 * @param fid Fid of the targeted queue
 * @param message Pointer to the location written by the function
 *
 * @return -1 if count value is not valid or if the queue got deleted or reset while the calling process was stuck.
 * Else 0.
 */
int preceive(int fid, int *message)
{
    if (fid < 0 || fid > NBQUEUE - 1 || !queue_table[fid])
    {
        return -1;
    }

    MessageQueue *q = queue_table[fid];
    if (q->nb_msg > 0) // Removing a message is the buffer is not empty
    {
        // Removing
        if (message)
        {
            *message = q->buffer[q->tail];
        }
        q->tail = q->tail + 1 > q->capacity - 1 ? 0 : q->tail + 1;

        // If the buffer was full and at least one process was waiting, complete the buffer with its message
        if (q->nb_msg == q->capacity && !queue_empty(&q->waiting_process))
        {
            Process *p = queue_out(&q->waiting_process, Process, link);
            q->buffer[q->head] = p->message; // p->message was set because psend had to be called by p
            q->head = q->head + 1 > q->capacity - 1 ? 0 : q->head + 1;

            p->state = ACTIVABLE;
            queue_add(p, &activable_list, Process, link, priority);

            process_table[current_pid]->state = ACTIVABLE;
            queue_add(process_table[current_pid], &activable_list, Process, link, priority);
            __scheduler();
        }
        else
        {
            q->nb_msg--;
        }
    }
    else // Blocking the process if the buffer is empty
    {
        Process *p = process_table[current_pid];
        p->state = IN_MSGQUEUE;
        queue_add(p, &q->waiting_process, Process, link, priority);

        __scheduler();

        // Code reaches here if the queue is deleted, reset
        // or if psend was called and this process was the least recent one in the message queue

        // If the queue was deleted or reset, return value is negative
        if (p->skip_message)
        {
            p->skip_message = false;
            return -1;
        }

        // If psend was called, we get here by a direct context switch to the least recent waiting process (current one)
        if (message)
        {
            *message = q->buffer[q->tail];
        }
        q->tail = q->tail + 1 > q->capacity - 1 ? 0 : q->tail + 1;
        q->nb_msg--;
    }

    return 0;
}

/**
 * @brief Resets a message queue.
 *
 * This function resets a message queue by making all the stuck process activable and setting the buffer pointers to 0.
 *
 * @param fid Fid of the targeted process
 *
 * @return -1 if fid value is not valid. Else 0.
 */
int preset(int fid)
{
    if (fid < 0 || fid > NBQUEUE - 1 || !queue_table[fid])
    {
        return -1;
    }

    Process *current;
    while (!queue_empty(&queue_table[fid]->waiting_process))
    {
        current = queue_out(&queue_table[fid]->waiting_process, Process, link);
        current->state = ACTIVABLE;
        current->skip_message = true;
        queue_add(current, &activable_list, Process, link, priority);
    }
    // No need to erase buffer's content since it will be overwritten
    queue_table[fid]->nb_msg = 0;
    queue_table[fid]->head = 0;
    queue_table[fid]->tail = 0;

    process_table[current_pid]->state = ACTIVABLE;
    queue_add(process_table[current_pid], &activable_list, Process, link, priority);
    __scheduler();

    return 0;
}

/**
 * @brief Gives information about a message queue.
 *
 * If the buffer is empty : Writes (-nb_waiting_process) in *count
 * Else : Writes (nb_msg + nb_waiting_process) in *count
 *
 * @param fid Fid of the targeted process
 * @param count A pointer to the location written by the function
 *
 * @return -1 if fid value is not valid. Else 0.
 */
int pcount(int fid, int *count)
{
    if (fid < 0 || fid > NBQUEUE - 1 || !queue_table[fid])
    {
        return -1;
    }
    if (count)
    {
        if (queue_table[fid]->nb_msg == 0)
        {
            *count = -__nb_waiting_proc(fid);
        }
        else
        {
            *count = __nb_waiting_proc(fid) + queue_table[fid]->nb_msg;
        }
    }
    return 0;
}

/**
 * @brief Returns the number of process blocked in a message queue.
 *
 * This function returns the number of process blocked in a message queue.
 *
 * @param fid Fid of the targeted process
 *
 * @return the number of process blocked in the queue identified by fid
 */
int __nb_waiting_proc(int fid)
{
    int n = 0;
    Process *current;
    queue_for_each(current, &queue_table[fid]->waiting_process, Process, link)
    {
        n++;
    }
    return n;
}

/**
 * @brief Returns the first free fid in the message queue table.
 *
 * This function returns the first free fid in the message queue table.
 * If no fid is available, it returns -1.
 *
 * @return The first free fid in the message queue table, or -1 if no fid is available.
 */
int __get_free_fid()
{
    int fid = -1;
    for (int i = 0; i < NBQUEUE; i++)
    {
        if (queue_table[i] == NULL)
        {
            fid = i;
            break;
        }
    }
    return fid;
}