#ifndef CONFIG_MEM_H
#define CONFIG_MEM_H

typedef struct
{
    char *puerto_escucha;
    int tam_memoria;
    int tam_pagina;
    char *path_instrucciones;
    int retardo_respuesta;
} t_mem_config;

extern t_mem_config *cfg_mem;

#endif