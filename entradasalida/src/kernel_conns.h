#ifndef IO_KERNEL_CONNS_H
#define IO_KERNEL_CONNS_H

#include <stdlib.h>
#include <stdio.h>
#include <proto/proto.h>
#include <proto/memory.h>
#include <proto/interface.h>
#include <string.h>
#include <math.h>
#include <commons/string.h>
#include <fcb.h>
#include <errno.h>
#include <commons/log.h>
#include <config.h>
#include <utils/utlis.h>
#include <prompt.h>
#include <blocks.h>

int registerResourceInKernel(int kernel_fd, t_log *logger, t_io_config *config);

struct kernel_incoming_message_args
{
    int kernel_fd;
    int memory_fd;
    t_log *logger;
    t_io_config *config;
};

void handleKernelIncomingMessage(uint8_t client_fd, uint8_t operation, t_buffer *buffer, void *_args);

#endif