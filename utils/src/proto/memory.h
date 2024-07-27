#ifndef MEMORY_PROTO_H
#define MEMORY_PROTO_H


#include <proto/proto.h>

typedef struct
{
    uint32_t pid;
    uint32_t total_bytes;
    t_list* access_list;
} t_memory_read_msg;

typedef struct
{
    void *value;
} t_memory_read_ok_msg;

typedef struct
{
    uint32_t pid;
    uint32_t total_bytes;
    t_list* access_list;
    void *value;
} t_memory_write_msg;

typedef struct {
    uint32_t pid;
    uint32_t page;
} t_page_table_access;

typedef struct {
    uint32_t address;
    uint32_t bytes_to_access;
} t_access_to_memory;

int memory_send_page_table_access(int fd, uint32_t pid, uint32_t page);
int memory_decode_page_table_access(t_buffer *buffer, t_page_table_access *access);

int memory_send_frame_number(t_page_table_access *access);
uint32_t memory_decode_frame_number(t_buffer *buffer);

int memory_send_read(int fd, uint32_t pid, t_list* access_list, uint32_t total_bytes);
void memory_decode_read(t_buffer *buffer, t_memory_read_msg *msg);
void memory_destroy_read(t_memory_read_msg *msg);

int memory_send_read_ok(int fd, void *value, uint32_t size);
void memory_decode_read_ok(t_buffer *buffer, t_memory_read_ok_msg *msg, uint32_t size);
void memory_destroy_read_ok(t_memory_read_ok_msg *msg);

void packet_add_access_to_mem(t_packet* packet,t_access_to_memory* access);
t_access_to_memory* packet_get_access_to_mem(t_buffer* buffer);

int memory_send_write(int fd, uint32_t pid, t_list* access_list, uint32_t total_bytes, void *value);
void memory_decode_write(t_buffer *buffer, t_memory_write_msg *msg);
void memory_destroy_write(t_memory_write_msg *msg);

int memory_send_write_ok(int fd);

#endif