#ifndef PCB_H
#define PCB_H

#include <stdlib.h>
#include <context/context.h>
#include<commons/collections/dictionary.h>
#include <config.h>

typedef enum
{
    NEW,
    READY,
    EXEC,
    BLOCKED,
    EXIT,
} t_state;

typedef struct
{
    t_exec_context *context;
    t_state state;
    char *text_path;
    t_dictionary* taken_resources;
    bool sigterm;
    // quizas algunas mas ahora no se
} t_pcb;

t_pcb *pcb_create(char* path);

void pcb_destroy(t_pcb *pcb);

char *pcb_state_to_string(t_pcb *pcb);

#endif