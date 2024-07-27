#ifndef FIFO_H
#define FIFO_H

#include <dispatcher.h>
#include <interrupt_comms.h>
#include <pcb.h>
#include <state_queues.h>
#include <commons/log.h>

t_pcb* ready_to_exec_fifo(void);
void dispatch_fifo(t_pcb* pcb, t_log* logger);
void block_to_ready_fifo(t_blocked_queue* queue,t_log* logger);
int move_pcb_to_blocked_fifo(t_pcb *pcb, char *resource_name, t_log *logger);

#endif