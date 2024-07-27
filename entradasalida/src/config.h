#ifndef IO_CONFIG_H
#define IO_CONFIG_H


typedef struct
{
    char *name;
    char *tipo_interfaz;
    int unidad_trabajo;
    char *ip_kernel;
    char *puerto_kernel;
    char *ip_memoria;
    char *puerto_memoria;
    char *path_base_dialfs;
    int block_size;
    int block_count;
    int retraso_compactacion;
} t_io_config;

extern t_io_config *cfg_io;

extern char *interface_name;


#endif