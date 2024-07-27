#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <context/context.h>
#include <global_context.h>

typedef struct
{
    char *name;
    void *address;
    size_t size;
} t_register;

t_register *register_get_by_name(char *name);

uint32_t register_get_value(t_register* reg);

#endif