#include <proto/proto.h>

t_packet *packet_new(uint8_t op_code)
{
    t_packet *packet = malloc(sizeof(t_packet));
    if (packet == NULL)
        return NULL;

    packet->op_code = op_code;
    packet->buffer = malloc(sizeof(t_buffer));
    packet->buffer->stream = NULL;
    packet->buffer->size = 0;

    return packet;
}

void packet_free(t_packet *packet)
{
    free(packet->buffer->stream);
    free(packet->buffer);
    free(packet);
}

int packet_add(t_packet *packet, void *value, int size)
{
    packet->buffer->stream = realloc(packet->buffer->stream, packet->buffer->size + size);

    memcpy(packet->buffer->stream + packet->buffer->size, value, size);

    packet->buffer->size += size;

    if (packet->buffer->stream == NULL)
        return -1;

    return 0;
}

void *packet_serialize(t_packet *packet, uint32_t bytes)
{
    void *serialized = malloc(bytes);
    int offset = 0;

    memcpy(serialized + offset, &(packet->op_code), sizeof(uint8_t));
    offset += sizeof(uint8_t);
    memcpy(serialized + offset, &(packet->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(serialized + offset, packet->buffer->stream, packet->buffer->size);
    offset += packet->buffer->size;

    return serialized;
}

int packet_send(t_packet *packet, int server_fd)
{
    uint32_t bytes = packet->buffer->size + sizeof(uint8_t) + sizeof(uint32_t);
    void *to_send = packet_serialize(packet, bytes);

    if (send(server_fd, to_send, bytes, 0) != bytes)
    {
        free(to_send);
        return -1;
    }

    free(to_send);
    return 0;
}

int packet_recv(int fd, t_packet *packet)
{
    if (recv(fd, &packet->op_code, sizeof(uint8_t), 0) != sizeof(uint8_t))
        return -1;

    uint32_t size;
    // RECIBO EL size DEL BUFFER
    if (recv(fd, &size, sizeof(uint32_t), 0) != sizeof(uint32_t))
        return -1;
    
    if(size == 0) return 0;

    packet->buffer->size = size;

    
    // log_debug(logger,"size: %d",packet->buffer->size);
    packet->buffer->stream = malloc(size);

    // RECIBO EL PAYLOAD
    if (recv(fd, packet->buffer->stream, size, 0) != size)
        return -1;

    return 0;
}

void packet_get(t_buffer *buffer, void *dest, int size)
{
    memcpy(dest, buffer->stream, size);
    buffer->size -= size;
    memmove(buffer->stream, buffer->stream + size, buffer->size);
    buffer->stream = realloc(buffer->stream, buffer->size);
}

int packet_addString(t_packet *packet, char *str)
{
    uint32_t length = strlen(str) + 1;
    packet_add(packet, &length, sizeof(uint32_t));
    packet_add(packet, str, length);

    return 0;
}

char *packet_getString(t_buffer *buffer)
{
    char *result;
    uint32_t size;
    packet_get(buffer, &size, sizeof(uint32_t));
    result = malloc(size);
    packet_get(buffer, result, size);
    return result;
}



int packet_addUInt32(t_packet *packet, uint32_t value)
{
    return packet_add(packet, &value, sizeof(uint32_t));
}

uint32_t packet_getUInt32(t_buffer *buffer)
{
    uint32_t value;
    packet_get(buffer, &value, sizeof(uint32_t));
    return value;
}

int packet_add_uint8(t_packet *packet, uint8_t value)
{
    return packet_add(packet, &value, sizeof(uint8_t));
}

uint8_t packet_get_uint8(t_buffer *buffer)
{
    uint8_t value;
    packet_get(buffer, &value, sizeof(uint8_t));
    return value;
}

int packet_add_list(t_packet* packet, t_list* list, void(*element_packer)(t_packet*, void*)){
    uint32_t elements_count = list->elements_count;
    packet_addUInt32(packet,elements_count);
    void add_to_packet(void* elem){
        element_packer(packet,elem);
    }
    list_iterate(list,add_to_packet);
    return 0;
}

t_list* packet_get_list(t_buffer* buffer, void*(*element_getter)(t_buffer*)){
    t_list* list = list_create();
    uint32_t size = packet_getUInt32(buffer);
    for (int i =0; i<size; i++){
        list_add(list,element_getter(buffer));
    }
    return list;
}


