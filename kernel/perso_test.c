#include "cpu.h"

#include "perso_test.h"
#include "screen.h"
#include "time.h"
#include "process.h"
#include "message_queue.h"

/*
int proc(void *arg)
{
    (void)arg;
    while (1)
    {
        printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
        sti();
        hlt();
        cli();
    }
}

int proc_phase4_wait_test(void *arg)
{
    (void)arg;
    int pid[4];
    pid[0] = start(proc_phase4_p1, 2048, 1, "p1", NULL);
    pid[1] = start(proc_phase4_p2, 2048, 1, "p2", NULL);
    pid[2] = start(proc_phase4_p3, 2048, 1, "p3", NULL);
    pid[3] = start(proc_phase4_p4, 2048, 1, "p4", NULL);
    wait_clock(frequency * 60);
    for (int i = 0; i < 4; i++)
    {
         kill(pid[i]);
    }
    return 0;
}

int proc_phase4_p1(void *arg)
{
    (void)arg;
    while (1) {
        printf(".");
        wait_clock(frequency * 1);

        sti();
        hlt();
        cli();
    }
    return 0;
}
int proc_phase4_p2(void *arg)
{
    (void)arg;
    while (1) {
        printf("-");
        wait_clock(frequency * 2);

        sti();
        hlt();
        cli();
    }
    return 0;
}
int proc_phase4_p3(void *arg)
{
    (void)arg;
    while (1) {
        printf("+");
        wait_clock(frequency * 5);

        sti();
        hlt();
        cli();
    }
    return 0;
}
int proc_phase4_p4(void *arg)
{
    (void)arg;
    while (1) {
        printf("*");
        wait_clock(frequency * 10);

        sti();
        hlt();
        cli();
    }
    return 0;
}

int proc_communication(void *arg)
{
    (void)arg;

    printf("TEST1\n");
    int fid = pcreate(1);
    int *params = mem_alloc(sizeof(int));
    params[0] = fid;
    int pid1 = start(proc_receiver_stuck_empty, 2048, 1, "proc2", (void*)params);
    int pid2 = start(proc_sender_stuck_empty, 2048, 1, "proc3", (void*)params);
    waitpid(pid1, NULL);
    waitpid(pid2, NULL);
    printf("TEST1 END\n\n");

    printf("TEST2\n");
    fid = pcreate(3);
    params[0] = fid;
    pid1 = start(proc_sender_stuck_full, 2048, 1, "proc4", (void*)params);
    pid2 = start(proc_receiver_stuck_full, 2048, 1, "proc5", (void*)params);
    waitpid(pid1, NULL);
    waitpid(pid2, NULL);
    printf("TEST2 END\n\n");

    printf("TEST3\n");
    fid = pcreate(1);
    params[0] = fid;
    pid1 = start(proc_receiver_unexpected_deletion, 2048, 1, "proc6", (void*)params);
    pid2 = start(proc_unexpected_delete, 2048, 1, "proc7", (void*)params);
    waitpid(pid1, NULL);
    waitpid(pid2, NULL);
    printf("TEST3 END\n\n");

    printf("TEST4\n");
    fid = pcreate(2);
    int p1[1] = {fid};
    start(proc_receiver, 2048, 1, "proc8", (void*)p1);
    int p2[1] = {fid};
    start(proc_receiver, 2048, 1, "proc9", (void*)p2);
    int p3[2] = {fid, 1};
    start(proc_sender, 2048, 1, "proc10", (void*)p3);
    int p4[1] = {fid};
    start(proc_receiver, 2048, 1, "proc11", (void*)p4);
    int p5[2] = {fid, 2};
    start(proc_sender, 2048, 1, "proc12", (void*)p5);
    int p6[2] = {fid, 3};
    start(proc_sender, 2048, 1, "proc13", (void*)p6);
    int p7[2] = {fid, 4};
    start(proc_sender, 2048, 1, "proc14", (void*)p7);
    int p8[1] = {fid};
    start(proc_receiver, 2048, 1, "proc15", (void*)p8);
    int p9[1] = {fid};
    start(proc_receiver, 2048, 1, "proc16", (void*)p9);
    wait_clock(200);
    int p10[1] = {fid};
    start(proc_queue_reset, 2048, 1, "proc17", (void*)p10);

    int ret = 0;
    while (ret >= 0) { ret = waitpid(-1, NULL); }
    printf("Waiting for children\n");

    int p11[2] = {fid, 5};
    start(proc_sender, 2048, 1, "proc18", (void*)p11);
    int p12[1] = {fid};
    start(proc_receiver, 2048, 1, "proc20", (void*)p12);
    int p13[1] = {fid};
    start(proc_receiver, 2048, 1, "proc21", (void*)p13);
    wait_clock(200);
    int p14[1] = {fid};
    start(proc_queue_delete, 2048, 1, "proc22", (void*)p14);

    ret = 0;
    while (ret >= 0) { ret = waitpid(-1, NULL); }
    printf("TEST4 END\n\n");
    return 0;
}

int proc_sender(void *arg)
{
    psend(((int *)arg)[0],((int *)arg)[1]);
    return 0;
}

int proc_receiver(void *arg)
{
    int *buffer = mem_alloc(sizeof(int));
    int ret = preceive(((int *)arg)[0], buffer);
    printf("ret : %d, message : %d\n", ret, *buffer);
    return 0;
}

int proc_queue_reset(void *arg)
{
    preset(((int *)arg)[0]);
    return 0;
}

int proc_queue_delete(void *arg) {
    pdelete(((int *)arg)[0]);
    return 0;
}

int proc_receiver_unexpected_deletion(void *arg)
{
    printf("in proc_receiver\n");
    printf("proc_receiver tries to receive and gets stuck in queue\n");
    int ret = preceive(((int*)arg)[0], NULL);
    printf("Back to proc_receiver, preceive return value %d\n", ret);
    printf("proc_receiver ends normally\n");
    return 0;
}

int proc_unexpected_delete(void *arg)
{
    printf("in proc_delete_queue\n");
    printf("proc_delete_queue starts waiting\n");
    wait_clock(1000);
    printf("proc_delete_queue deletes the queue\n");
    pdelete(((int*)arg)[0]);
    printf("proc_delete_queue ends normally\n");
    return 0;
}

int proc_sender_stuck_full(void *arg)
{
    printf("in proc_sender\n");
    psend(((int*)arg)[0], 42);
    psend(((int*)arg)[0], 43);
    psend(((int*)arg)[0], 44);
    printf("proc_sender sends to full buffer, scheduler is called\n");
    psend(((int*)arg)[0], 45);
    pdelete(((int*)arg)[0]);
    printf("back to proc_sender, deletes the queue and ends normally\n");
    return 0;
}

int proc_receiver_stuck_full(void *arg)
{
    printf("in proc_receiver\n");
    int *b = queue_table[((int*)arg)[0]]->buffer;

    printf("proc_receiver starts waiting\n");
    wait_clock(1000);
    printf("Buffer expected 42 43 44 : %d %d %d\n", b[0], b[1], b[2]);
    printf("proc_receiver removes a message, writes the message of the stuck sending process and unlocks it\n");
    preceive(((int*)arg)[0], NULL);
    printf("Buffer expected 45 43 44 : %d %d %d\n", b[0], b[1], b[2]);
    printf("proc_receiver ends normally\n");
    return 0;
}

int proc_sender_stuck_empty(void *arg)
{
    printf("in proc_sender\n");
    printf("proc_sender starts waiting\n");
    wait_clock(1000);
    printf("proc_sender sends the message and instantly hands over to proc_receiver\n");
    int ret = psend(((int*)arg)[0], 42);
    printf("back to proc_sender, message sent with ret %i\n", ret);
    printf("proc_sender ends normally\n");
    return 0;
}

int proc_receiver_stuck_empty(void *arg)
{
    printf("in proc_receiver\n");
    int *buffer = mem_alloc(sizeof(int));
    printf("proc_receiver gets stuck in the message queue, scheduler is called\n");
    int ret = preceive(((int*)arg)[0], buffer);
    printf("proc_receiver received the message %i with ret %i\n", *buffer, ret);
    ret = pdelete(((int*)arg)[0]);
    printf("proc_receiver deleted the queue with ret %i then ends normally\n", ret);
    return 0;
}

int proc_waitpid_test(void *arg) {
    (void)arg;
    printf("START\n");
    int p[1] = {1000};
    start(proc_simple_wait, 2048, 1, "proc1", (void *)p);
    waitpid(-1, NULL);
    printf("END\n");

    return 0;
}

int proc_child_of_child(void *arg)
{
    (void)arg;
    printf("[%s] I'm a child of a child\n", process_table[current_pid]->name);
    while (1) {
        // wait_clock(200);
        sti();
        hlt();
        cli();
    }
}

int proc_child(void *arg)
{
    (void)arg;
    start(proc_child_of_child, 2048, 2, "child of child", NULL);

    printf("[%s] I'm a child\n", process_table[current_pid]->name);
    wait_clock(200);
    printf("[%s] I'm exiting and becoming a zombie\n", process_table[current_pid]->name);
    exit(10);

    return 0;
}

int proc_father(void *arg)
{
    (void)arg;
    int first_child_pid = start(proc_child, 2048, 2, "1st child", NULL);
    printf("[%s] I'm the father\n", process_table[current_pid]->name);
    printf("[%s] I'm waiting for my child\n", process_table[current_pid]->name);

    int *retvalp = mem_alloc(8);
    waitpid(first_child_pid, retvalp);
    printf("[%s] Found my child, and they had a retval of %i\n", process_table[current_pid]->name, *retvalp);

    printf("[%s] I'm exiting, my child should be cleaned up\n", process_table[current_pid]->name);
    exit(0);

    return 0;
}

int proc_loop_wait(void *arg)
{
    (void)arg;
    while (1)
    {
        printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
        wait_clock(200);

        sti();
        hlt();
        cli();
    }

    return 0;
}

int proc_incremental_prio(void *arg)
{
    (void)arg;
    while (1)
    {
        printf("[%s] pid -> %i, prio -> %i\n", process_table[current_pid]->name, current_pid, getprio(current_pid));
        chprio(current_pid, process_table[current_pid]->priority + 1);
        wait_clock(200);

        sti();
        hlt();
        cli();
    }

    return 0;
}

int proc_prio(void *arg)
{
    (void)arg;
    while (1)
    {
        printf("[%s] pid = %i, prio -> %i\n", process_table[current_pid]->name, current_pid, getprio(current_pid));
        wait_clock(200);

        sti();
        hlt();
        cli();
    }

    return 0;
}

int proc_with_parameter(void *arg)
{
    (void)arg;
    while (1)
    {
        printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
        printf("[%s] Parameter = %i\n", process_table[current_pid]->name, ((int *)arg)[0]);
        wait_clock(800);
        sti();
        hlt();
        cli();
    }

    return 0;
}

int proc_end(void *arg)
{
    (void)arg;
    printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
    printf("[%s] Ending normally\n", process_table[current_pid]->name);
    return 0;
}

int proc_simple_wait(void *arg)
{
    printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
    printf("[%s] Start\n", process_table[current_pid]->name);
    wait_clock(((int*)arg)[0]);
    printf("[%s] End\n", process_table[current_pid]->name);

    return 0;
}

int proc_exit(void *arg)
{
    (void)arg;
    printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
    printf("[%s] Ending on exit call\n", process_table[current_pid]->name);
    exit(0);
    return 0;
}

int proc_suicidal(void *arg)
{
    (void)arg;
    printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
    printf("[%s] Ending killing itself\n", process_table[current_pid]->name);
    kill(current_pid);
    return 0;
}

int proc_killer(void *arg)
{
    printf("[%s] pid = %i\n", process_table[current_pid]->name, current_pid);
    wait_clock(1000);
    printf("[%s] Killing proc4\n", process_table[current_pid]->name);
    kill(((int *)arg)[0]);
    printf("[%s] Ending normally\n", process_table[current_pid]->name);
    return 0;
}
*/