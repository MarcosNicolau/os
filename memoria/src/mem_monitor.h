#ifndef MEM_MONITOR_H
#define MEM_MONITOR_H

#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

int init_ram(int size);

void read_mem(uint32_t physical_address, void * value, uint32_t size);
void write_mem(uint32_t physical_address, void * value, uint32_t size);

#endif