#ifndef MEM_COMMS_CPU
#define MEM_COMMS_CPU

#include <proto/proto.h>
#include<commons/log.h>
#include<stdlib.h>
#include <global_context.h>

extern int fd_memory;

extern uint32_t PAGE_SIZE;

void send_mem_handshake(t_log* logger);

uint32_t access_page_table(uint32_t page_number);

#endif