#ifndef MMU_H
#define MMU

#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <commons/string.h>
#include <tlb.h>
#include <sys/socket.h>
#include <proto/proto.h>
#include <commons/log.h>
#include <proto/memory.h>
#include <global_context.h>
#include <memory_comms.h>

t_list* access_to_memory_create(uint32_t logical_address, uint32_t total_bytes, uint32_t page_size, t_log *logger);

uint32_t get_frame(uint32_t page_number, t_log *logger);

#endif