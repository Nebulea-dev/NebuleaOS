#ifndef SRC_DE_BASE_PROCESSUS_H
#define SRC_DE_BASE_PROCESSUS_H

#include "stddef.h"
#include "stdbool.h"
#include "stdint.h"
#include "mem.h"
#include "queue.h"

#define NBPROC 30
#define MAXPRIO 256
#define MAXCHILDREN 8
#define DEFAULT_STACK_SIZE 512

typedef enum State
{
    ELECTED,
    ACTIVABLE,
    ASLEEP,
    ZOMBIE,
    FINISHED,
    WAITING_PID,
    IN_MSGQUEUE,
    IN_SEM
} State;

typedef enum Register_name
{
    EBX = 0,
    ESP = 1,
    EBP = 2,
    ESI = 3,
    EDI = 4,
    CR3 = 5,
} Register_name;

typedef struct Child
{
    int pid;
    int priority;
    link link;
} Child;

typedef struct Process
{
    // Process description
    int pid;
    char *name;
    int *kernel_stack;
    int *user_stack;
    int registers[5];
    enum State state;

    // Data to free the stack
    int *initial_kernel_stack;
    int *initial_user_stack;
    int kernel_stack_size;
    int user_stack_size;

    // Priority
    int priority;

    // Waking up information if asleep
    unsigned long wakeup_date;

    // Position in current queue
    link link;

    // Children
    int father_pid;
    link children;

    // Pid of the process the father was waiting (if waitpid was called with -1)
    int waited_pid;

    // Message
    int message;
    bool skip_message;

    // Semaphore
    int waited_sem;

    // Return value
    int retval;

    // Virtual memory
    int cr3;
} Process;

extern int current_pid;
extern Process *process_table[NBPROC];
extern link activable_list;
extern link finished_list;
extern link waiting_pid_list;

extern void ctx_sw(void *, void *);
extern void update_cr3(int);
void virtual_ctx_sw(Process *old_process, Process *new_process);

int __get_free_pid(void);
Process *__init_idle(void);
void __process_end(int pid);
void __scheduler(void);
void __wakeup_process(void);
void __free_process(Process *p);
void __free_children(Process *p);
void __free_stack(Process *p);
void __free_finished_process(void);
void __remove_from_children(int pid, int father_pid);
void __stack_exit(void);
void __awake_father(int child_pid);
void __finish_zombie(int pid, int *retvalp);
void __finish_fatherless(int pid);
bool __is_valid_pid(int pid);
link *__get_queue(Process *process);
int idle(void *arg);

// Public API
// int start(int (*pt_func)(void *), unsigned long ssize, int prio, const char *name, void *arg);
int start(const char *name, unsigned long ssize, int prio, void *arg);
int getprio(int pid);
int chprio(int pid, int newprio);
void exit(int retval);
int kill(int pid);
int waitpid(int pid, int *retvalp);
int getpid(void);
struct uapps get_symbol_from_name(const char *name);

#endif // SRC_DE_BASE_PROCESSUS_H
