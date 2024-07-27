#ifndef INTERFACE_PROTO_H
#define INTERFACE_PROTO_H

#include <proto/proto.h>
#include <proto/memory.h>

typedef struct
{
    char *name;
    char *type;
} t_interface_new_msg;

typedef struct
{
    uint32_t pid;
    char *interface_name;
} t_interface_io_done_msg;

typedef struct
{
    uint32_t work_units;
} t_interface_io_gen_sleep_msg;

typedef struct
{
    t_list *access_list;
    uint32_t size;
} t_interface_io_stdin_read_msg;

typedef struct
{
    t_list *access_list;
    uint32_t size;
} t_interface_io_stdout_write_msg;

typedef struct
{
    char* file_name;
} t_interface_io_dialfs_create_msg;

typedef struct
{
    char* file_name;
    t_list *access_list;
    uint32_t size;
    uint32_t file_pointer;
} t_interface_io_dialfs_read_msg;

typedef struct
{
    char* file_name;
    t_list *access_list;
    uint32_t size;
    uint32_t file_pointer;
} t_interface_io_dialfs_write_msg;

typedef struct
{
    char* file_name;
    uint32_t size;
} t_interface_io_dialfs_truncate_msg;

typedef struct
{
    char* file_name;
} t_interface_io_dialfs_del_msg;

// =========== GENERAL =============
int interface_send_io_error(int fd, char *interface_name, uint32_t pid, uint8_t error_code);
int interface_send_io_done(int fd, char *interface_name, uint32_t pid);
void interface_decode_io_done(t_buffer *buffer, t_interface_io_done_msg *msg);
void interface_destroy_io_done(t_interface_io_done_msg *msg);
int interface_send_new_interface(int fd, uint32_t pid, uint32_t work_units);
void interface_decode_new(t_buffer *buffer, t_interface_new_msg *interface);
void interface_destroy_new(t_interface_new_msg *interface);

// =========== IO GEN SLEEP =============
t_packet *interface_serialize_io_gen_sleep(uint32_t pid, uint32_t work_units);
void interface_decode_io_gen_sleep(t_buffer *buffer, t_interface_io_gen_sleep_msg *msg);
void interface_destroy_io_gen_sleep(t_interface_io_gen_sleep_msg *msg);

// =========== IO STDIN READ =============
t_packet *interface_serialize_io_stdin_read(uint32_t pid, t_interface_io_stdin_read_msg *msg);
void interface_decode_io_stdin_read(t_buffer *buffer, t_interface_io_stdin_read_msg *msg);
void interface_destroy_io_stdin_read(t_interface_io_stdin_read_msg *msg);

// =========== IO STDOUT WRITE  =============
t_packet *interface_serialize_io_stdout_write(uint32_t pid, t_interface_io_stdout_write_msg *msg);
void interface_decode_io_stdout_write(t_buffer *buffer, t_interface_io_stdout_write_msg *msg);
void interface_destroy_io_stdout_write(t_interface_io_stdout_write_msg *msg);

// =========== IO DIAL FS  =============
t_packet *interface_serialize_io_dialfs_read(uint32_t pid, t_interface_io_dialfs_read_msg *msg);
void interface_decode_io_dialfs_read(t_buffer *buffer, t_interface_io_dialfs_read_msg *msg);
void interface_destroy_io_dialfs_read(t_interface_io_dialfs_read_msg *msg);

t_packet *interface_serialize_io_dialfs_write(uint32_t pid, t_interface_io_dialfs_write_msg *msg);
void interface_decode_io_dialfs_write(t_buffer *buffer, t_interface_io_dialfs_write_msg *msg);
void interface_destroy_io_dialfs_write(t_interface_io_dialfs_write_msg *msg);

t_packet *interface_serialize_io_dialfs_truncate(uint32_t pid, t_interface_io_dialfs_truncate_msg *msg);
void interface_decode_io_dialfs_truncate(t_buffer *buffer, t_interface_io_dialfs_truncate_msg *msg);
void interface_destroy_io_dialfs_truncate(t_interface_io_dialfs_truncate_msg *msg);

t_packet *interface_serialize_io_dialfs_del(uint32_t pid, t_interface_io_dialfs_del_msg *msg);
void interface_decode_io_dialfs_del(t_buffer *buffer, t_interface_io_dialfs_del_msg *msg);
void interface_destroy_io_dialfs_del(t_interface_io_dialfs_del_msg *msg);

t_packet *interface_serialize_io_dialfs_create(uint32_t pid, t_interface_io_dialfs_create_msg *msg);
void interface_decode_io_dialfs_create(t_buffer *buffer, t_interface_io_dialfs_create_msg *msg);
void interface_destroy_io_dialfs_create(t_interface_io_dialfs_create_msg *msg);


#endif