#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <interface.h>
#include <pcb.h>
#include <commons/log.h>
#include <scheduler.h>
#include<commons/temporal.h>

extern int fd_dispatch;

extern pthread_t quantum_interruption_thread;
extern t_temporal *timer;
extern int time_elapsed;

void send_context_to_cpu(t_exec_context *context);

int wait_for_dispatch_reason(t_pcb *pcb, t_log *logger);


#endif