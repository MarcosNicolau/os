#ifndef TLB_H
#define TLB_H

#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils/utlis.h>
#include <memory_comms.h>
#include <math.h>
#include <time.h>
#include <string.h>

typedef enum {
    FIFO_TLB,
    LRU_TLB
} t_tlb_algorithm;

typedef struct {
    t_list *entries;
    uint32_t entries_amount;
    t_tlb_algorithm algorithm;
} t_tlb;

typedef struct {
    uint32_t pid;
    uint32_t page;
    uint32_t frame;
    long long last_use;
} t_tlb_row;

void tlb_init(int entries_amount, char *tlb_algorithm);

int tlb_search(uint32_t pid, uint32_t page, t_log *logger); // returns -1 if not found

void tlb_insert(uint32_t pid, uint32_t page, uint32_t frame,t_log* logger);

void cleanup_tlb(uint32_t pid, uint32_t size);

void tlb_dump(t_log* logger);

#endif