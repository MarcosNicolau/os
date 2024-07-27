#ifndef RR_H
#define RR_H

#include <dispatcher.h>
#include <interrupt_comms.h>
#include <pcb.h>
#include<stdio.h>
#include <state_queues.h>
#include <commons/log.h>
#include <fifo.h>
#include <commons/temporal.h>

t_pcb* ready_to_exec_rr(void);

void dispatch_rr(t_pcb* pcb, t_log* logger);
void block_to_ready_rr(t_blocked_queue* queue,t_log* logger);
void exec_to_ready_rr(t_pcb *pcb, t_log *logger); 
int move_pcb_to_blocked_rr(t_pcb *pcb, char *resource_name, t_log *logger);
void quantum_interruption_handler(void *args);

#endif