#ifndef INSTR_SET_CPU_H
#define INSTR_SET_CPU_H

#include <string.h>
#include <stdlib.h>
#include <register.h>
#include <commons/log.h>
#include <memory_comms.h>
#include <dispatch.h>
#include <interrupt.h>
#include <mmu.h>

typedef struct
{
    char *name;
    void (*instr)(char **args, t_log *logger);
} t_instruction;

extern t_instruction INSTRUCTION_SET[];
t_instruction *instruction_get_by_name(char *name);

extern int current_exec_process_has_finished;

#endif