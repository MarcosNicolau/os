#ifndef FCB_H
#define FCB_H

#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdlib.h>
#include <utils.h>
#include <stdio.h>
#include <blocks.h>
#include <bitmap_monitor.h>
#include <dirent.h>
#include <bitmap_monitor.h>

#define METADATA_BLOQUE_INICIAL "BLOQUE_INICIAL"
#define METADATA_TAMANIO_ARCHIVO "TAMANIO_ARCHIVO"


typedef struct {
    int first_block;
    int size;
} t_fcb;

void init_fcbs(void);
void create_file(char* file_name);
void delete_file(char *file_name);
bool file_already_exists(char* file_name);
void fcb_set_size(char* file_name,uint32_t size);
void fcb_set_first_block(char* file_name,int block);
t_fcb* get_metadata(char* file_name);
void compact(char *file_name, int target_blocks);

#endif