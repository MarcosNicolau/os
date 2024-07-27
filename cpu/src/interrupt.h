#ifndef COMM_INTR_H_
#define COMM_INTR_H_

#include<proto/proto.h>
#include<sockets/sockets.h>
#include<commons/log.h>


extern uint8_t interrupt_reason;

void handle_interrupt(void *_args);

void set_interrupt(void);

void clear_interrupt(void);

//returns value of interrupt_flag
int interrupt_flag(void);

void interrupt_mutex_init(void);

void interrupt_mutex_destroy(void);

#endif