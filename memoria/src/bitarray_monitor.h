#ifndef BITARRAY_MONITOR_MEM_H
#define BITARRAY_MONITOR_MEM_H

#include <stdlib.h>
#include <string.h>
#include <commons/bitarray.h>
#include <pthread.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

void init_mem_bitarray(int mem_size, int frame_size);

bool test_frame(uint32_t frame_number);

void set_frame_ocuppied(uint32_t frame_number);

void clear_frame(uint32_t frame_number);

#endif