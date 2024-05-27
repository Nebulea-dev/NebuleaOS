#ifndef PSYS_BASE_INFO_H
#define PSYS_BASE_INFO_H

#include "process.h"

#define SHOW_ZOMBIES 0
#define INFO_SIZE 6
#define QUEUE_TABLE_TITLE_SIZE 14
#define PROCESS_TABLE_TITLE_SIZE 16
#define ACTIVABLE_TITLE_SIZE 19
#define FINISHED_TITLE_SIZE 17
#define BUFFER_SIZE 128

char *state_to_string(enum State state);
void write_info(int, int, char *, int);
void write_info_titles(void);
void write_process_table(void);
void write_queue_table(void);
void write_activable(void);
void write_finished(void);

void sys_info(void);

#endif // PSYS_BASE_INFO_H
