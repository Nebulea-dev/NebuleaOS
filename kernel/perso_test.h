#ifndef SRC_DE_BASE_PERSO_TEST_H
#define SRC_DE_BASE_PERSO_TEST_H

int proc(void *arg);
int proc_phase4_p1(void *arg);
int proc_phase4_p2(void *arg);
int proc_phase4_p3(void *arg);
int proc_phase4_p4(void *arg);
int proc_phase4_wait_test(void *arg);
int proc_communication(void *arg);
int proc_sender(void *arg);
int proc_receiver(void *arg);
int proc_queue_reset(void *arg);
int proc_queue_delete(void *arg);
int proc_unexpected_delete(void *arg);
int proc_receiver_unexpected_deletion(void *arg);
int proc_sender_stuck_full(void *arg);
int proc_receiver_stuck_full(void *arg);
int proc_sender_stuck_empty(void *arg);
int proc_receiver_stuck_empty(void *arg);
int proc_waitpid_test(void *arg);
int proc_child_of_child(void *arg);
int proc_child(void *arg);
int proc_father(void *arg);
int proc_incremental_prio(void *arg);
int proc_with_parameter(void *arg);
int proc_prio(void *arg);
int proc_end(void *arg);
int proc_exit(void *arg);
int proc_suicidal(void *arg);
int proc_killer(void *arg);
int proc_loop_wait(void *arg);
int proc_simple_wait(void *arg);

#endif 