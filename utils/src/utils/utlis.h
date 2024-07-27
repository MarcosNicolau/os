#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <commons/config.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

#define BUFFER_MAX_LENGTH 100

/**
 * @fn    file_get_list_of_lines
 * @brief de un archivo, devuelve una lista con todas las lineas sin el '\n'
 */
t_list *file_get_list_of_lines(char *file_path);

/**
 * @fn    file_get_nth_line
 * @brief de un archivo, devuelve la enesima linea sin el '\n' arranca en 0
 */
char *file_get_nth_line(char *file_path, int n);

int file_exists(char *file_path);

int msleep(long msec);

long long current_timestamp(void);

char *mount_config_directory(char* module_name,char *config_path);

bool config_has_all_properties(t_config* config, char** properties);

#endif