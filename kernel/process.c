#include "cpu.h"
#include "string.h"
#include "stdbool.h"
#include "queue.h"
#include "userspace_apps.h"
#include "processor_structs.h"

#include "message_queue.h"
#include "semaphore.h"
#include "process.h"
#include "time.h"
#include "mmu.h"
#include "consts.h"
#include "usermode.h"

// Pid of the current process
int current_pid = -1;

// Reserved stack size for the scheduler actions (read documentation)
const int SCHEDULER_STACK_SIZE = 2048;

// Table of processes
Process *process_table[NBPROC];

// Linked list of activable processes
link activable_list = LIST_HEAD_INIT(activable_list);
link finished_list = LIST_HEAD_INIT(finished_list);
link waiting_pid_list = LIST_HEAD_INIT(waiting_pid_list);

// Page directory
extern unsigned pgdir[];
extern struct x86_tss tss;

/**
 * @brief Idle function, does nothing except halting the processor
 *
 * This function is the Idle function,it does nothing except halting the processor
 */
int idle(void *arg)
{
    (void)arg;
    printf("Idle process started\n");
    while (1)
    {
        sti();
        hlt();
        cli();
    }
}

int start(const char *name, unsigned long ssize, int prio, void *arg)
{
    // Check if ssize is too big
    if (ssize > 0x10000000)
    {
        return -1;
    }

    Process *p = mem_alloc(sizeof(Process));

    // 1 - A l'aide du nom de programme, reçu par start, on localise le code du programme à charger
    struct uapps process_symbols = get_symbol_from_name(name);
    int size_to_map = (int)(process_symbols.end - process_symbols.start);

    // 2- Allocation du la mémoire physique pour le code et la pile.
    void *process_code = virtual_malloc(size_to_map);

    // 3 - Créer un page directory et une page table pour réaliser les mappings mémoire.
    unsigned *new_pgdir = allocate_physical_memory_address();
    // void *new_page_table = virtual_malloc(4096);

    // 4 - Mapper le kernel.
    fill_new_pgdir(new_pgdir, pgdir);

    // 5 - Copier le code et le mapper à partir de l'adresse 1Go
    memcpy(process_code, process_symbols.start, size_to_map);
    pgdir_copy(pgdir, new_pgdir, process_code, ONE_GIGABYTE, size_to_map);

    // 6 - Mapper la pile à l'adresse 2Go.
    p->user_stack_size = ssize;
    p->user_stack = virtual_malloc(ssize + 8) + ssize + 4;
    pgdir_copy(pgdir, new_pgdir, p->user_stack, TWO_GIGABYTES, ssize + 8);

    // 7 - Associer le page directory au processus et le démarrer.
    p->cr3 = (int)new_pgdir;

    // Process index in the table is its pid
    int pid = __get_free_pid();
    if (pid == -1 || prio > MAXPRIO)
    {
        mem_free(p, sizeof(Process));
        return -1;
    }
    process_table[pid] = p;
    p->pid = pid;

    // Dynamically allocation for the name
    p->name = mem_alloc(strlen(name) + 1);
    strcpy(p->name, name);

    // Create kernel stack
    p->kernel_stack_size = 8192;
    p->initial_kernel_stack = mem_alloc(p->kernel_stack_size);
    p->kernel_stack = p->initial_kernel_stack + p->kernel_stack_size - 8;

    // Setup kernel stack
    *(p->kernel_stack--) = (int)arg;
    *(p->kernel_stack--) = (int)NULL;
    *(p->kernel_stack) = (int)new_process_start_function;

    p->registers[ESP] = (int)(p->kernel_stack);
    p->registers[EBP] = p->registers[ESP];

    // TODO c'est dégeulasse, pourquoi -9 ?
    *(p->user_stack-- - 9) = (int)arg;

    pgdir_copy(pgdir, new_pgdir, p->user_stack, TWO_GIGABYTES, ssize + 8);

    p->user_stack = TWO_GIGABYTES + ssize - 4;

    p->priority = prio;

    // Initialize hierarchy of processes
    p->father_pid = current_pid;
    INIT_LIST_HEAD(&p->children);

    // Add process to father's children list
    if (p->father_pid != -1)
    {
        Child *child = mem_alloc(sizeof(Child));
        child->pid = p->pid;
        child->priority = 1;
        queue_add(child, &(process_table[p->father_pid]->children), Child, link, priority);
    }

    // If the priority of the new process is greater than the current's, it is elected instantly.
    // Else, insertion of the new process in the activable list
    if (current_pid >= 0 && p->priority > process_table[current_pid]->priority)
    {
        // Low priority process is unelected and added the activable list
        Process *old_process = process_table[current_pid];
        old_process->state = ACTIVABLE;
        queue_add(old_process, &activable_list, Process, link, priority);

        // High priority process is elected
        p->state = ELECTED;
        current_pid = p->pid;
        virtual_ctx_sw(old_process, p);
    }
    else
    {
        p->state = ACTIVABLE;
        queue_add(p, &activable_list, Process, link, priority);
    }

    // Becomes true if the process was stuck in a message queue which was deleted or reset
    p->skip_message = false;

    return p->pid;
}

/**
 * @brief Returns the pid of the current process.
 *
 * This function returns the pid of the current process.
 *
 * @param pid The pid of the current process
 */
int getprio(int pid)
{
    if (!__is_valid_pid(pid) || process_table[pid]->state == ZOMBIE)
    {
        return -1;
    }
    return process_table[pid]->priority;
}

int getpid()
{
    return current_pid;
}

/**
    @brief Changes the priority of a process.

    This function changes the priority of a process.

    @param pid The pid of the process
    @param newprio The new priority of the process
    @return The former priority value of the process, or -1 if the process does not exist or if the new priority is too large
*/
int chprio(int pid, int newprio)
{
    Process *p = process_table[pid];

    // Returns -1 if pid describes no process or if newprio is too large
    if (!__is_valid_pid(pid) || newprio > MAXPRIO || newprio <= 0 || pid == 0 || p->state == ZOMBIE || p->state == FINISHED)
    {
        return -1;
    }

    // Returns former priority value either
    int old_prio = p->priority;
    // If the process is in a queue, it must be removed, its priority changed, then put back in the queue
    if (!queue_initialized(&p->link))
    {
        link *queue = __get_queue(p);
        assert(queue != NULL);

        queue_del(p, link);

        p->priority = newprio;
        queue_add(p, queue, Process, link, priority);
    }
    else
    {
        p->priority = newprio;
    }

    // Not changing current pid for the scheduler to be able to switch context.
    // The scheduler will possibly switch from a context to the same one.
    // Takes in account : newprio > current prio ; current newprio < any other prio
    process_table[current_pid]->state = ACTIVABLE;
    queue_add(process_table[current_pid], &activable_list, Process, link, priority);
    __scheduler();

    return old_prio;
}

/**
 * @brief Suspends the current process and sets its wakeup date.
 *
 * This function suspends the current process and sets its wakeup date.
 *
 * @param wakeup The date at which the process will be woken up
 */
void wait_clock(unsigned long wakeup)
{
    process_table[current_pid]->state = ASLEEP;
    process_table[current_pid]->wakeup_date = wakeup;
    __scheduler();
}

/**
 * @brief Exits the current process.
 *
 * This function exits the current process.
 *
 * @param retval The return value of the process
 */
void exit(int retval)
{
    process_table[current_pid]->retval = retval;
    __process_end(current_pid);

    // Exit function conflicts with std exit function (stdlib.h)
    // So we need to never return (don't ask why)
    while (1)
    {
    }
}

/**
 * @brief Kills a process.
 *
 * This function kills a process.
 *
 * @param pid The pid of the process
 * @return 0 if the process was killed, -1 if the process does not exist
 */
int kill(int pid)
{
    // Check if process is killable
    if (!__is_valid_pid(pid) || pid == 0 || process_table[pid]->state == ZOMBIE)
    {
        return -1;
    }

    // The killed process can be in a queue from which it must be removed
    if (!queue_initialized(&process_table[pid]->link))
    {
        Process *proc = process_table[pid];
        queue_del(proc, link);

        if (proc->state == IN_SEM)
        {
            Semaphore *sem = semaphore_table[proc->waited_sem];
            sem->count++;
            // signal(sem->sid);
        }
    }

    process_table[pid]->retval = 0;
    __process_end(pid);

    return 0;
}

int waitpid(int pid, int *retvalp)
{
    if (retvalp && (unsigned)retvalp < (unsigned)0x4000000)
    {
        return -1;
    }

    if (pid >= NBPROC)
    {
        return -1;
    }

    if (pid < 0)
    {

        if (queue_empty(&process_table[current_pid]->children))
        {
            return -1;
        }

        // For each child of the current process
        Child *current;
        bool found = false;
        queue_for_each(current, &process_table[current_pid]->children, Child, link)
        {
            // If process is ended and waiting for a waitpid
            if (process_table[current->pid]->state == ZOMBIE)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            __finish_zombie(current->pid, retvalp);
            return current->pid;
        }

        // If no child is ended, wait for one
        process_table[current_pid]->state = WAITING_PID;
        queue_add(process_table[current_pid], &waiting_pid_list, Process, link, priority);

        __scheduler();

        // After the father is woken up, the waited child is finished
        int finished_pid = process_table[current_pid]->waited_pid;
        __finish_zombie(finished_pid, retvalp);
        return finished_pid;
    }

    else
    {
        // Check if pid is a child of current process
        if (process_table[pid] == NULL || process_table[pid]->father_pid != current_pid)
        {
            return -1;
        }

        // If process is ended and waiting for a waitpid
        if (process_table[pid]->state == ZOMBIE)
        {
            __finish_zombie(pid, retvalp);
            return pid;
        }
        else
        {
            // If the child is not ended, wait for it
            process_table[current_pid]->state = WAITING_PID;
            queue_add(process_table[current_pid], &waiting_pid_list, Process, link, priority);

            __scheduler();

            // After the father is woken up, the waited child is finished
            __finish_zombie(pid, retvalp);
            return pid;
        }
    }
}

link *__get_queue(Process *process)
{
    switch (process->state)
    {
    case ELECTED:
        return NULL;
    case ACTIVABLE:
        return &activable_list;
    case ASLEEP:
        return NULL;
    case ZOMBIE:
        return NULL;
    case FINISHED:
        return NULL;
    case WAITING_PID:
        return &waiting_pid_list;
    case IN_MSGQUEUE:
        for (int fid = 0; fid < NBQUEUE; fid++)
        {
            if (queue_table[fid])
            {
                Process *current;
                queue_for_each(current, &queue_table[fid]->waiting_process, Process, link)
                {
                    if (current->pid == process->pid)
                    {
                        return &queue_table[fid]->waiting_process;
                    }
                }
            }
        }
        return NULL;
    case IN_SEM:
        for (int sem = 0; sem < NBSEMAPHORE; sem++)
        {
            if (semaphore_table[sem])
            {
                Process *current;
                queue_for_each(current, &semaphore_table[sem]->waiting_process, Process, link)
                {
                    if (current->pid == process->pid)
                    {
                        return &semaphore_table[sem]->waiting_process;
                    }
                }
            }
        }
    }
    return NULL;
}

/**
 * @brief Function called when a process ends.
 *
 * @param pid The pid of the process
 *
 * This function is called when a process ends, no matter how.
 */
void __process_end(int pid)
{
    Process *ending_process = process_table[pid];
    if (ending_process->father_pid != -1)
    {
        // Turn process into a zombie
        ending_process->state = ZOMBIE;

        // If this process' father was waiting for it to end, it wakes up
        __awake_father(pid);
    }
    else
    {
        __finish_fatherless(pid);
    }

    // Mark children as fatherless and kill zombie children
    Child *current_child;
    queue_for_each(current_child, &ending_process->children, Child, link)
    {
        process_table[current_child->pid]->father_pid = -1;
        if (process_table[current_child->pid]->state == ZOMBIE)
        {
            __finish_fatherless(current_child->pid);
        }
    }

    if (pid == current_pid)
    {
        __scheduler();
    }
}

void __finish_zombie(int pid, int *retvalp)
{
    // If retvalp is not NULL, use it to store the return value
    if (retvalp)
    {
        if ((unsigned)retvalp < (unsigned)0x4000000)
        {
            panic("Dirty hacker stopped");
        }
        else
        {
            *retvalp = process_table[pid]->retval;
        }
    }

    // Finish the zombie : removes it from its father's children list, adds it to be freed and releases its pid
    __remove_from_children(pid, current_pid);
    process_table[pid]->state = FINISHED;
    queue_add(process_table[pid], &finished_list, Process, link, priority);
    process_table[pid] = NULL;
}

void __finish_fatherless(int pid)
{
    // Finish the process : adds it to be freed and releases its pid
    process_table[pid]->state = FINISHED;
    queue_add(process_table[pid], &finished_list, Process, link, priority);
    process_table[pid] = NULL;
}

/**
 * @brief Removes a process from its father's children list.
 *
 * @param pid The pid of the process to be removed
 * @param father_pid The pid of the father of the process to be removed
 */
void __remove_from_children(int pid, int father_pid)
{
    // If process has no father
    if (father_pid == -1)
    {
        return;
    }

    Process *father = process_table[father_pid];
    Child *current;
    bool found = false;
    queue_for_each(current, &father->children, Child, link)
    {
        if (current->pid == pid)
        {
            found = true;
            break;
        }
    }
    if (found)
    {
        queue_del(current, link);
        // TODO : We can't free here because we will return the child pid in waitpid
        // mem_free(current, sizeof(Child));
    }
}

/**
 * @brief Default exit function for processes.
 *
 * This function exits the current process with the return value stored in EAX.
 */
void __stack_exit()
{
    int return_value = -1;
    __asm__ __volatile__("movl %%eax, %0" : "=r"(return_value));

    exit(return_value);
}

/**
 * @brief Frees the memory allocated for a process.
 *
 * This function frees the memory allocated for a process
 * and removes it from the process table.
 *
 * Cannot be called by the process itself, or else the function will
 * free its own stack, crashing the kernel
 *
 * @param p A pointer to the process to be freed.
 */
void __free_process(Process *p)
{
    if (p->pid == current_pid)
    {
        panic("Trying to free the current process");
    }

    mem_free(p->initial_kernel_stack, p->kernel_stack_size);
    mem_free(p->name, strlen(p->name) + 1);
    mem_free(p, sizeof(Process));
}

/**
 * @brief Returns the first free pid in the process table.
 *
 * This function returns the first free pid in the process table.
 * If no pid is available, it returns -1.
 *
 * @return The first free pid in the process table, or -1 if no pid is available.
 */
int __get_free_pid()
{
    int pid = -1;
    for (int i = 0; i < NBPROC; i++)
    {
        if (process_table[i] == NULL)
        {
            pid = i;
            break;
        }
    }
    return pid;
}

/**
 * @brief Initializes the idle process.
 *
 * This function initializes the idle process.
 */
Process *__init_idle()
{
    Process *p_idle = virtual_malloc(sizeof(Process));

    process_table[0] = p_idle;
    p_idle->pid = 0;

    p_idle->name = virtual_malloc(5);
    strcpy(p_idle->name, "idle");

    p_idle->kernel_stack = virtual_malloc(2048);
    p_idle->kernel_stack_size = 512;
    p_idle->kernel_stack[511] = (int)ONE_GIGABYTE;
    p_idle->registers[ESP] = (int)&(p_idle->kernel_stack[511]);
    p_idle->registers[EBP] = (int)&(p_idle->kernel_stack[511]);

    // 1 - A l'aide du nom de programme, reçu par start, on localise le code du programme à charger
    struct uapps process_symbols = get_symbol_from_name("idle");
    int size_to_map = (int)(process_symbols.end - process_symbols.start);

    // 5 - Copier le code et le mapper à partir de l'adresse 1Go
    map_region(pgdir, (unsigned)ONE_GIGABYTE, PAGE_TABLE_USER_RW, size_to_map);
    memcpy(ONE_GIGABYTE, process_symbols.start, size_to_map);

    map_region(pgdir, (unsigned)TWO_GIGABYTES, PAGE_TABLE_USER_RW, 4096 * 10);
    p_idle->user_stack_size = 1024 * 10;
    p_idle->user_stack = TWO_GIGABYTES + p_idle->user_stack_size;
    p_idle->cr3 = (int)pgdir;

    p_idle->state = ELECTED;

    // Lowest priority
    p_idle->priority = 1;

    return p_idle;
}

/**
 * @brief Scheduler code, called by the timer interrupt.
 *
 * This function frees zombie processes, wakes up sleeping processes
 * and chooses a new process to run.
 *
 * It then performs a context switch to the new process.
 */
void __scheduler()
{
    // Free finished process (except the current one if it is finished already)
    __free_finished_process();

    // Wake up sleeping process if possible
    __wakeup_process();

    // Chose a process from priority queue
    Process *new_process = queue_out(&activable_list, Process, link);

    new_process->state = ELECTED;

    // If the finished queue is not empty, the current process is finished.
    // In this case, its pid was released. As we can't access it through the process table,
    // we must find it as the sole element of the finished queue.
    Process *old_process = queue_empty(&finished_list) ? process_table[current_pid] : queue_top(&finished_list, Process, link);
    current_pid = new_process->pid;

    // Context switch
    // printf("Switching from %s to %s\n", old_process->name, new_process->name);
    // printf("Switching from PID %d to PID %d\n", old_process->pid, new_process->pid);
    virtual_ctx_sw(old_process, new_process);
}

/**
 * @brief Frees all finished processes.
 *
 * This function frees all finished processes.
 */
void __free_finished_process()
{
    Process *current;
    bool found = false;
    queue_for_each(current, &finished_list, Process, link)
    {
        if (current->pid == current_pid)
        {
            found = true;
            break;
        }
    }
    if (found)
    {
        queue_del(current, link);
    }

    Process *finished;
    while (!queue_empty(&finished_list))
    {
        finished = queue_out(&finished_list, Process, link);
        __free_process(finished);
    }

    if (found)
    {
        queue_add(current, &finished_list, Process, link, priority);
    }
}

bool __is_valid_pid(int pid)
{
    return pid < NBPROC && pid >= 0 && process_table[pid] != NULL;
}

/**
 * @brief Wakes up sleeping processes if their wakeup date has been reached.
 *
 * This function wakes up sleeping processes if their wakeup date has been reached.
 */
void __wakeup_process()
{
    for (int i = 0; i < NBPROC; i++)
    {
        if (process_table[i] && process_table[i]->state == ASLEEP && process_table[i]->wakeup_date <= current_clock())
        {
            process_table[i]->state = ACTIVABLE;
            queue_add(process_table[i], &activable_list, Process, link, priority);
        }
    }
}

void __awake_father(int child_pid)
{
    Process *current_waiting;
    bool found = false;
    queue_for_each(current_waiting, &waiting_pid_list, Process, link)
    {
        if (current_waiting->pid == process_table[child_pid]->father_pid)
        {
            found = true;
            break;
        }
    }
    if (found)
    {
        queue_del(current_waiting, link);
        current_waiting->waited_pid = child_pid;
        current_waiting->state = ACTIVABLE;
        queue_add(current_waiting, &activable_list, Process, link, priority);
    }
}

struct uapps get_symbol_from_name(const char *name)
{
    extern const struct uapps symbols_table[];
    int i = 0;
    while (symbols_table[i].name != NULL)
    {
        if (strcmp(symbols_table[i].name, name) == 0)
        {
            return symbols_table[i];
        }
        i++;
    }
    panic("symbol pas trouvé, a l'aideeeeeeeeee !!!!!!!!\n");
}

void virtual_ctx_sw(Process *old_process, Process *new_process)
{
    // Switch CR3
    update_cr3(new_process->cr3);
    tss.cr3 = new_process->cr3;

    // Update esp0 for new process to be able to move to ring0
    tss.esp0 = (int)(new_process->kernel_stack + new_process->kernel_stack_size);

    // Context switch
    ctx_sw(old_process->registers, new_process->registers);
}