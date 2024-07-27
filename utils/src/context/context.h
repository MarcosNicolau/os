#ifndef CONTEXT_UTILS_H
#define CONTEXT_UTILS_H

#include <stdint.h>
#include <proto/proto.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    uint32_t pc;
    uint8_t ax;
    uint8_t bx;
    uint8_t cx;
    uint8_t dx;
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;
    uint32_t di;
} t_cpu_registers;

typedef struct
{
    uint32_t pid;
    t_cpu_registers registers;
    uint32_t quantum;
} t_exec_context;

void packet_get_context(t_buffer *buffer, t_exec_context *context);

void packet_add_context(t_packet *packet, t_exec_context *context);

void packet_get_registers(t_buffer *buffer, t_cpu_registers *registers);

void packet_add_registers(t_packet *packet, t_cpu_registers *registers);

#endif