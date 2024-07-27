#include <register.h>


t_register REGISTERS[] = {{"PC", &context.registers.pc, sizeof(uint32_t)},
                          {"AX", &context.registers.ax, sizeof(uint8_t)},
                          {"BX", &context.registers.bx, sizeof(uint8_t)},
                          {"CX", &context.registers.cx, sizeof(uint8_t)},
                          {"DX", &context.registers.dx, sizeof(uint8_t)},
                          {"EAX", &context.registers.eax, sizeof(uint32_t)},
                          {"EBX", &context.registers.ebx, sizeof(uint32_t)},
                          {"ECX", &context.registers.ecx, sizeof(uint32_t)},
                          {"EDX", &context.registers.edx, sizeof(uint32_t)},
                          {"SI", &context.registers.si, sizeof(uint32_t)},
                          {"DI", &context.registers.di, sizeof(uint32_t)},
                          {NULL, NULL, 0}};

t_register *register_get_by_name(char *name)
{
    for (uint8_t i = 0; REGISTERS[i].name != NULL; i++)
    {
        if (!strcmp(REGISTERS[i].name, name))
            return &REGISTERS[i];
    }
    return NULL;
}

uint32_t register_get_value(t_register* reg){
    uint32_t* value = calloc(1,sizeof(uint32_t));
    memcpy(value,reg->address,reg->size);
    uint32_t result = *value;
    free(value);
    return result;
}