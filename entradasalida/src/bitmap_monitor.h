#ifndef BITMAP_MONITOR_H
#define BITMAP_MONITOR_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <commons/bitarray.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <commons/log.h>
#include <utils.h>
#include <sys/mman.h>

void load_bitmap(t_log * logger);

int find_first_free_block(void);

void close_bitmap(void);

void occupy_free_blocks(int from, int block_count);

void free_blocks(int from, int block_count);

uint32_t total_free_blocks(void);

uint32_t free_contiguous_blocks_from(int file_end);

bool test_bit_from_bitmap(int bit);

#endif
