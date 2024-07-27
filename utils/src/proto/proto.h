#ifndef PROTO_H
#define PROTO_H

#include <stdlib.h>
#include <stdint.h>
#include <sockets/sockets.h>
#include <commons/string.h>
#include <utils/utlis.h>

typedef struct
{
    uint32_t size;
    void *stream;
} t_buffer;

typedef enum
{
    // OPERACIONES KERNEL-CPU
    EXEC_PROCESS,
    INTERRUPT_EXEC,
    SAVE_CONTEXT,
    RESTORE_CONTEXT,
    END_PROCESS,
    END_OF_QUANTUM,
    WAIT,
    SIGNAL,
    // OPERACIONES CPU/INTERFACES-MEMORIA
    CPU_HANDSHAKE,
    NEXT_INSTRUCTION,
    NO_INSTRUCTION,
    READ_MEM,
    READ_MEM_OK,
    WRITE_MEM,
    WRITE_MEM_OK,
    RESIZE_PROCESS,
    RESIZE_OK,
    R_EXTEND_OK,
    R_SHRINK_OK,
    OUT_OF_MEMORY,
    GET_FRAME,
    // OPERACIONES KERNEL-MEMORIA
    CREATE_PROCESS,
    ERROR_CREATING_PROCESS,
    DESTROY_PROCESS,
    PAGE_FRAME,
    // OPERACIONES INTERFACES-KERNEL
    NEW_INTERFACE,
    IO_GEN_SLEEP,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    IO_DONE,
    IO_ERROR,
} t_opcode;

typedef struct
{
    uint8_t op_code;
    t_buffer *buffer;

} t_packet;

t_packet *packet_new(uint8_t op_code);
void packet_free(t_packet *packet);

/**
 * @fn    packet_get
 * @brief agrega al final del stream otro stream de bytes de tam size
 */
int packet_add(t_packet *packet, void *value, int size);

int packet_send(t_packet *packet, int client_fd);

/**
 * @fn    packet_get
 * @brief del principio del stream saca una parte de tam size y lo guarda en dest
 */
void packet_get(t_buffer *buffer, void *dest, int size);

int packet_recv(int fd, t_packet *packet);

uint32_t packet_getUInt32(t_buffer *buffer);
uint8_t packet_get_uint8(t_buffer *buffer);
char *packet_getString(t_buffer *buffer);


int packet_addUInt32(t_packet *packet, uint32_t value);
int packet_add_uint8(t_packet *packet, uint8_t value);
int packet_addString(t_packet *packet, char *str);

int packet_add_list(t_packet* packet, t_list* list, void(*element_packer)(t_packet*, void*));

t_list* packet_get_list(t_buffer* buffer, void*(*element_getter)(t_buffer*));

typedef void (*t_requestHandler)(uint8_t client_fd, uint8_t operation, t_buffer *buffer, void *args);
int socket_read(int fd, t_requestHandler requestHandler, void *args);

#endif