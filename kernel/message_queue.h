#ifndef PSYS_BASE_MESSAGE_H
#define PSYS_BASE_MESSAGE_H

#include "stddef.h"
#include "queue.h"

#define NBQUEUE 10
#define QUEUE_MAX_SIZE 1000

/**
 * A MessageQueue is implemented by :
 * - A queue of process blocked waiting (either to send or to receive a message)
 * - A circular buffer of messages
 */
typedef struct MessageQueue {
    int fid;
    int capacity; // Buffer max size
    int nb_msg; // Buffer current number of messages
    int head; // Buffer writing index
    int tail; // Buffer reading index
    int *buffer; // Buffer
    link waiting_process; // Queue of blocked process
} MessageQueue;

extern MessageQueue *queue_table[NBQUEUE];

int __get_free_fid(void);
int __nb_waiting_proc(int fid);

// Public API
int pcreate(int count);
int pdelete(int fid);
int psend(int fid, int message);
int preceive(int fid,int *message);
int preset(int fid);
int pcount(int fid, int *count);

#endif // PSYS_BASE_MESSAGE_H