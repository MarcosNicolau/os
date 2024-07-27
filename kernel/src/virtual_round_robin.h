#ifndef VRR_H
#define VRR_H

#include <dispatcher.h>
#include <interrupt_comms.h>
#include <pcb.h>
#include <stdio.h>
#include <state_queues.h>
#include <commons/log.h>
#include <round_robin.h>
#include <commons/temporal.h>

t_pcb* ready_to_exec_vrr(void);

void dispatch_vrr(t_pcb* pcb, t_log* logger);
void block_to_ready_vrr(t_blocked_queue* queue,t_log* logger);
void exec_to_ready_vrr(t_pcb *pcb, t_log *logger); 
int move_pcb_to_blocked_vrr(t_pcb *pcb, char *resource_name, t_log *logger);

#endif