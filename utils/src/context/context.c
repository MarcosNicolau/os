#include <context/context.h>

void packet_get_context(t_buffer *buffer, t_exec_context *context)
{
    context->pid = packet_getUInt32(buffer);
    packet_get_registers(buffer, &context->registers);
    context->quantum = packet_getUInt32(buffer);
}

void packet_add_context(t_packet *packet, t_exec_context *context)
{
    packet_addUInt32(packet, context->pid);
    packet_add_registers(packet, &context->registers);
    packet_addUInt32(packet, context->quantum);
}

void packet_get_registers(t_buffer *buffer, t_cpu_registers *registers)
{
    registers->pc = packet_getUInt32(buffer);
    registers->ax = packet_get_uint8(buffer);
    registers->bx = packet_get_uint8(buffer);
    registers->cx = packet_get_uint8(buffer);
    registers->dx = packet_get_uint8(buffer);
    registers->eax = packet_getUInt32(buffer);
    registers->ebx = packet_getUInt32(buffer);
    registers->ecx = packet_getUInt32(buffer);
    registers->edx = packet_getUInt32(buffer);
    registers->si = packet_getUInt32(buffer);
    registers->di = packet_getUInt32(buffer);
}

void packet_add_registers(t_packet *packet, t_cpu_registers *registers)
{
    packet_addUInt32(packet, registers->pc);
    packet_add_uint8(packet, registers->ax);
    packet_add_uint8(packet, registers->bx);
    packet_add_uint8(packet, registers->cx);
    packet_add_uint8(packet, registers->dx);
    packet_addUInt32(packet, registers->eax);
    packet_addUInt32(packet, registers->ebx);
    packet_addUInt32(packet, registers->ecx);
    packet_addUInt32(packet, registers->edx);
    packet_addUInt32(packet, registers->si);
    packet_addUInt32(packet, registers->di);
}