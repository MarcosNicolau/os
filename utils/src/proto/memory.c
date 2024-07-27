#include <proto/memory.h>

void packet_add_access_to_mem(t_packet* packet,t_access_to_memory* access){
    packet_addUInt32(packet,access->address); // direccion fisica
    packet_addUInt32(packet,access->bytes_to_access);
}

t_access_to_memory* packet_get_access_to_mem(t_buffer* buffer){
    t_access_to_memory* access = malloc(sizeof(t_access_to_memory));
    access->address = packet_getUInt32(buffer);
    access->bytes_to_access = packet_getUInt32(buffer);
    return access;
}

int memory_send_read(int fd, uint32_t pid, t_list* access_list, uint32_t total_bytes)
{
    t_packet *packet = packet_new(READ_MEM);
    packet_addUInt32(packet, pid);
    packet_addUInt32(packet, total_bytes);
    packet_add_list(packet, access_list, (void*) packet_add_access_to_mem);
    int res = packet_send(packet, fd);
    packet_free(packet);
    return res;
}

void memory_decode_read(t_buffer *buffer, t_memory_read_msg *msg)
{
    msg->pid = packet_getUInt32(buffer);
    msg->total_bytes = packet_getUInt32(buffer);
    msg->access_list = packet_get_list(buffer,(void*) packet_get_access_to_mem);
}

void memory_destroy_read(t_memory_read_msg *msg)
{
    list_destroy_and_destroy_elements(msg->access_list,free);
    free(msg);
}

int memory_send_read_ok(int fd, void *value, uint32_t size)
{
    t_packet *packet = packet_new(READ_MEM_OK);
    packet_add(packet, value, size);
    int res = packet_send(packet, fd);
    packet_free(packet);
    return res;
}

void memory_decode_read_ok(t_buffer *buffer, t_memory_read_ok_msg *msg, uint32_t size)
{
    msg->value = malloc(size);
    packet_get(buffer, msg->value, size);
}

void memory_destroy_read_ok(t_memory_read_ok_msg *msg)
{
    free(msg->value);
    free(msg);
}

int memory_send_write(int fd, uint32_t pid, t_list* access_list, uint32_t total_bytes, void *value)
{
    t_packet *packet = packet_new(WRITE_MEM);
    packet_addUInt32(packet, pid);
    packet_addUInt32(packet,total_bytes);
    packet_add_list(packet,access_list,(void*)packet_add_access_to_mem);
    packet_add(packet,value,total_bytes);
    int res = packet_send(packet, fd);
    packet_free(packet);
    return res;
}

void memory_decode_write(t_buffer *buffer, t_memory_write_msg *msg)
{
    msg->pid = packet_getUInt32(buffer);
    msg->total_bytes = packet_getUInt32(buffer);
    msg->access_list = packet_get_list(buffer,(void*)packet_get_access_to_mem);
    msg->value = malloc(msg->total_bytes);
    packet_get(buffer, msg->value,msg->total_bytes);
}

void memory_destroy_write(t_memory_write_msg *msg)
{
    free(msg->value);
    list_destroy_and_destroy_elements(msg->access_list,free);
    free(msg);
}

int memory_send_write_ok(int fd)
{
    t_packet *packet = packet_new(WRITE_MEM_OK);
    int res = packet_send(packet, fd);
    packet_free(packet);
    return res;
}

int memory_send_page_table_access(int fd, uint32_t pid, uint32_t page) {return 1;}
int memory_decode_page_table_access(t_buffer *buffer, t_page_table_access *access) {return 1;} 

int memory_send_frame_number(t_page_table_access *access) {return 1;}
uint32_t memory_decode_frame_number(t_buffer *buffer) {return 1;}