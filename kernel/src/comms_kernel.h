#ifndef COMMUNICATIONS_H_KERNEL
#define COMMUNICATIONS_H_KERNEL

#include <proto/proto.h>
#include <sockets/sockets.h>
#include <interface.h>
#include <semaphore.h>
#include <state_queues.h>
#include <scheduler.h>

void handle_connections(void *void_args);

#endif