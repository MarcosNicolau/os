#ifndef CONFIG_K_H
#define CONFIG_K_H

typedef struct
{
    char *puerto_escucha;
    char *ip_memoria;
    char *puerto_memoria;
    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    char *algoritmo_planificacion;
    int quantum;
    char **recursos;
    char **instancias_recursos;
    int grado_multiprogramacion;
    char* pathbase_scripts;
} t_kernel_config;

extern t_kernel_config *cfg_kernel;

#endif