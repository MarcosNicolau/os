#include <memory_comms.h>

int fd_memory;

uint32_t PAGE_SIZE;

void send_mem_handshake(t_log* logger)
{
    t_packet *packet = packet_new(CPU_HANDSHAKE);
    packet_send(packet, fd_memory);
    packet_free(packet);
    packet = packet_new(-1);
    if(packet_recv(fd_memory,packet) == -1){
        log_error(logger,"Error al comunicarme con la memoria");
        exit(1);
    }
    PAGE_SIZE = packet_getUInt32(packet->buffer);
    packet_free(packet);
    log_info(logger,"conexion incial con memoria exitosa, tam pagina: %d",PAGE_SIZE);
}

uint32_t access_page_table(uint32_t page_number){
    t_packet* packet = packet_new(GET_FRAME);
    packet_addUInt32(packet, context.pid);
    packet_addUInt32(packet, page_number);
    packet_send(packet,fd_memory);
    packet_free(packet);

    packet = packet_new(-1);
    if(packet_recv(fd_memory,packet)==-1){
        packet_free(packet);
        return -1;
    }
    uint32_t frame = packet_getUInt32(packet->buffer);
    packet_free(packet);
    return frame;
}
