#include "info.h"
#include "screen.h"
#include "time.h"
#include "message_queue.h"

char *state_to_string(enum State state)
{
    switch (state)
    {
    case ELECTED:
        return "ELECTED";
    case ACTIVABLE:
        return "ACTIVABLE";
    case ASLEEP:
        return "ASLEEP";
    case ZOMBIE:
        return "ZOMBIE";
    case FINISHED:
        return "FINISHED";
    case WAITING_PID:
        return "WAITING_PID";
    case IN_MSGQUEUE:
        return "IN_MSGQUEUE";
    default:
        return "";
    }
}

void write_info(int line, int shift, char *string, int len)
{
    if (len <= 80 - shift)
    {
        for (int i = 0; i < len; i++)
        {
            write_car(line, shift + i, string[i], foreground_color, background_color);
        }
    }
}

void write_info_titles()
{
    write_info(1, 0, "Process table : ", PROCESS_TABLE_TITLE_SIZE);
    write_info(2, 0, "Queue table : ", QUEUE_TABLE_TITLE_SIZE);
    write_info(3, 0, "Activables queue : ", ACTIVABLE_TITLE_SIZE);
    write_info(4, 0, "Finished queue : ", FINISHED_TITLE_SIZE);
}

void write_process_table()
{
    for (int i = 0; i < NB_COLUMNS; i++)
    {
        write_car(1, PROCESS_TABLE_TITLE_SIZE + i, 32, 15, 0);
    }
    int position = PROCESS_TABLE_TITLE_SIZE;
    for (int i = 0; i < NBPROC; i++)
    {
        if (process_table[i] && (process_table[i]->state != ZOMBIE || SHOW_ZOMBIES))
        {
            char buffer[BUFFER_SIZE];
            int len = sprintf(buffer, "%d:%s ", i, state_to_string(process_table[i]->state));
            write_info(1, position, buffer, len);
            position += len;
        }
    }
}

void write_queue_table()
{
    for (int i = 0; i < NB_COLUMNS; i++)
    {
        write_car(2, QUEUE_TABLE_TITLE_SIZE + i, 32, 15, 0);
    }
    int position = QUEUE_TABLE_TITLE_SIZE;
    for (int i = 0; i < NBQUEUE; i++)
    {
        if (queue_table[i])
        {
            char buffer[BUFFER_SIZE];
            int len = sprintf(buffer, "%d ", queue_table[i]->fid);
            write_info(2, position, buffer, len);
            position += len;
        }
    }
}

void write_activable()
{
    for (int i = 0; i < NB_COLUMNS; i++)
    {
        write_car(3, ACTIVABLE_TITLE_SIZE + i, 32, 15, 0);
    }
    Process *current;
    int position = ACTIVABLE_TITLE_SIZE;
    queue_for_each(current, &activable_list, Process, link)
    {
        char buffer[BUFFER_SIZE];
        int len = sprintf(buffer, "%d ", current->pid);
        write_info(3, position, buffer, len);
        position += len;
    }
}

void write_finished()
{
    for (int i = 0; i < NB_COLUMNS; i++)
    {
        write_car(4, FINISHED_TITLE_SIZE + i, 32, 15, 0);
    }
    Process *current;
    int position = FINISHED_TITLE_SIZE;
    queue_for_each(current, &finished_list, Process, link)
    {
        char buffer[BUFFER_SIZE];
        int len = sprintf(buffer, "%d ", current->pid);
        write_info(4, position, buffer, len);
        position += len;
    }
}

void sys_info(void)
{
    // TODO : changer ca pour l'afficher autre part que en bandeau au dessus de l'ecran ?
    write_process_table();
    write_queue_table();
    write_activable();
    write_finished();

    /*
     * Pour la soutenance, devrait afficher la liste des processus actifs, des
     * files de messages utilisees et toute autre info utile sur le noyau.
     */
    /*
        case SYS_INFO:
            int proc_number = 0;
            for (size_t i = 0; i < NBPROC; i++)
            {
                if (process_table[i]) {proc_number++;}
            }
            printf("number of process : %d\n", proc_number);

            printf("activable process list :\n");
            Process *current;
            queue_for_each(current, &activable_list, Process, link)
            {
                printf("%d; ", current->pid);
            }

            printf("\nrunning for : %d\n", current_clock());
    */
}
