#include "interface.h"

// =========== GENERAL =============

int interface_send_io_error(int fd, char *interface_name, uint32_t pid, uint8_t error_code)
{
    t_packet *packet = packet_new(error_code);
    packet_addString(packet, interface_name);
    packet_addUInt32(packet, pid);
    int res = packet_send(packet, fd);
    packet_free(packet);
    return res;
}

int interface_send_io_done(int fd, char *interface_name, uint32_t pid)
{
    t_packet *packet = packet_new(IO_DONE);
    packet_addString(packet, interface_name);
    packet_addUInt32(packet, pid);
    int res = packet_send(packet, fd);
    packet_free(packet);
    return res;
}

void interface_decode_io_done(t_buffer *buffer, t_interface_io_done_msg *msg)
{
    msg->interface_name = packet_getString(buffer);
    msg->pid = packet_getUInt32(buffer);
}

void interface_destroy_io_done(t_interface_io_done_msg *msg)
{
    free(msg->interface_name);
    free(msg);
}

int interface_send_new_interface(int fd, uint32_t pid, uint32_t work_units)
{
    t_packet *packet = packet_new(NEW_INTERFACE);
    packet_addUInt32(packet, pid);
    packet_addUInt32(packet, work_units);
    int res = packet_send(packet, fd);
    packet_free(packet);
    return res;
}

void interface_decode_new(t_buffer *buffer, t_interface_new_msg *interface)
{
    interface->name = packet_getString(buffer);
    interface->type = packet_getString(buffer);
}

void interface_destroy_new(t_interface_new_msg *interface)
{
    free(interface->name);
    free(interface->type);
    free(interface);
}

// =========== IO GEN SLEEP =============
t_packet *interface_serialize_io_gen_sleep(uint32_t pid, uint32_t work_units)
{
    t_packet *packet = packet_new(IO_GEN_SLEEP);
    packet_addUInt32(packet, pid);
    packet_addUInt32(packet, work_units);
    return packet;
}

void interface_decode_io_gen_sleep(t_buffer *buffer, t_interface_io_gen_sleep_msg *msg)
{
    msg->work_units = packet_getUInt32(buffer);
}

void interface_destroy_io_gen_sleep(t_interface_io_gen_sleep_msg *msg)
{
    free(msg);
}

// =========== IO STDIN READ =============
t_packet *interface_serialize_io_stdin_read(uint32_t pid, t_interface_io_stdin_read_msg *msg)
{
    t_packet *packet = packet_new(IO_STDIN_READ);
    packet_addUInt32(packet, pid);
    packet_add_list(packet, msg->access_list, (void *)packet_add_access_to_mem);
    packet_addUInt32(packet, msg->size);
    return packet;
}

void interface_decode_io_stdin_read(t_buffer *buffer, t_interface_io_stdin_read_msg *msg)
{
    msg->access_list = packet_get_list(buffer, (void *)packet_get_access_to_mem);
    msg->size = packet_getUInt32(buffer);
}

void interface_destroy_io_stdin_read(t_interface_io_stdin_read_msg *msg)
{
    list_destroy_and_destroy_elements(msg->access_list, free);
    free(msg);
}

// =========== IO STDOUT WRITE  =============
t_packet *interface_serialize_io_stdout_write(uint32_t pid, t_interface_io_stdout_write_msg *msg)
{
    t_packet *packet = packet_new(IO_STDOUT_WRITE);
    packet_addUInt32(packet, pid);
    packet_add_list(packet, msg->access_list, (void *)packet_add_access_to_mem);
    packet_addUInt32(packet, msg->size);
    return packet;
}

void interface_decode_io_stdout_write(t_buffer *buffer, t_interface_io_stdout_write_msg *msg)
{
    msg->access_list = packet_get_list(buffer, (void *)packet_get_access_to_mem);
    msg->size = packet_getUInt32(buffer);
}

void interface_destroy_io_stdout_write(t_interface_io_stdout_write_msg *msg)
{
    list_destroy_and_destroy_elements(msg->access_list, free);
    free(msg);
}

// =========== IO DIAL FS READ =============
t_packet *interface_serialize_io_dialfs_read(uint32_t pid, t_interface_io_dialfs_read_msg *msg)
{
    t_packet *packet = packet_new(IO_FS_READ);
    packet_addUInt32(packet, pid);
    packet_addString(packet, msg->file_name);
    packet_add_list(packet, msg->access_list, (void *)packet_add_access_to_mem);
    packet_addUInt32(packet, msg->size);
    packet_addUInt32(packet, msg->file_pointer);
    return packet;
}

void interface_decode_io_dialfs_read(t_buffer *buffer, t_interface_io_dialfs_read_msg *msg)
{
    msg->file_name = packet_getString(buffer);
    msg->access_list = packet_get_list(buffer, (void *)packet_get_access_to_mem);
    msg->size = packet_getUInt32(buffer);
    msg->file_pointer = packet_getUInt32(buffer);
}

void interface_destroy_io_dialfs_read(t_interface_io_dialfs_read_msg *msg)
{
    free(msg->file_name);
    list_destroy_and_destroy_elements(msg->access_list, free);
    free(msg);
}

// =========== IO DIAL FS WRITE =============
t_packet *interface_serialize_io_dialfs_write(uint32_t pid, t_interface_io_dialfs_write_msg *msg)
{
    t_packet *packet = packet_new(IO_FS_WRITE);
    packet_addUInt32(packet, pid);
    packet_addString(packet, msg->file_name);
    packet_add_list(packet, msg->access_list, (void *)packet_add_access_to_mem);
    packet_addUInt32(packet, msg->size);
    packet_addUInt32(packet, msg->file_pointer);
    return packet;
}

void interface_decode_io_dialfs_write(t_buffer *buffer, t_interface_io_dialfs_write_msg *msg)
{
    msg->file_name = packet_getString(buffer);
    msg->access_list = packet_get_list(buffer, (void *)packet_get_access_to_mem);
    msg->size = packet_getUInt32(buffer);
    msg->file_pointer = packet_getUInt32(buffer);
}

void interface_destroy_io_dialfs_write(t_interface_io_dialfs_write_msg *msg)
{
    free(msg->file_name);
    list_destroy_and_destroy_elements(msg->access_list, free);
    free(msg);
}

// =========== IO DIAL FS CREATE =============
t_packet *interface_serialize_io_dialfs_create(uint32_t pid, t_interface_io_dialfs_create_msg *msg)
{
    t_packet *packet = packet_new(IO_FS_CREATE);
    packet_addUInt32(packet, pid);
    packet_addString(packet, msg->file_name);
    return packet;
}

void interface_decode_io_dialfs_create(t_buffer *buffer, t_interface_io_dialfs_create_msg *msg)
{
    msg->file_name = packet_getString(buffer);
}

void interface_destroy_io_dialfs_create(t_interface_io_dialfs_create_msg *msg)
{
    free(msg->file_name);
    free(msg);
}


// =========== IO DIAL FS TRUNCATE =============
t_packet *interface_serialize_io_dialfs_truncate(uint32_t pid, t_interface_io_dialfs_truncate_msg *msg)
{
    t_packet *packet = packet_new(IO_FS_TRUNCATE);
    packet_addUInt32(packet, pid);
    packet_addString(packet, msg->file_name);
    packet_addUInt32(packet, msg->size);
    return packet;
}

void interface_decode_io_dialfs_truncate(t_buffer *buffer, t_interface_io_dialfs_truncate_msg *msg)
{
    msg->file_name = packet_getString(buffer);
    msg->size = packet_getUInt32(buffer);
}

void interface_destroy_io_dialfs_truncate(t_interface_io_dialfs_truncate_msg *msg)
{
    free(msg->file_name);
    free(msg);
}

// =========== IO DIAL FS DELETE =============
t_packet *interface_serialize_io_dialfs_del(uint32_t pid, t_interface_io_dialfs_del_msg *msg)
{
    t_packet *packet = packet_new(IO_FS_DELETE);
    packet_addUInt32(packet, pid);
    packet_addString(packet, msg->file_name);
    return packet;
}

void interface_decode_io_dialfs_del(t_buffer *buffer, t_interface_io_dialfs_del_msg *msg)
{
    msg->file_name = packet_getString(buffer);
}

void interface_destroy_io_dialfs_del(t_interface_io_dialfs_del_msg *msg)
{ 
    free(msg->file_name);
    free(msg);
}