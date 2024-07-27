#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include <pcb.h>
#include <state_queues.h>
#include <memory_comms.h>
#include<utils.h>
#include <scheduler.h>

void init_process(char *path, t_log *logger);
void end_process(char *pid, t_log *logger);
void stop_scheduler(char *, t_log *logger);
void start_scheduler(char *, t_log *logger);
void multiprogramming(char *value, t_log *logger);
void list_processes_by_state(char *, t_log *logger);
void list_resources(char *x, t_log *logger);

#endif