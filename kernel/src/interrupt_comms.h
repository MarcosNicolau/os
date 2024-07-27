#ifndef INTERRUPT_K_H
#define INTERRUPT_K_H

#include<proto/proto.h>
#include<dispatcher.h>
#include<pcb.h>

extern int fd_interrupt;

void send_interrupt(uint8_t reason);

#endif