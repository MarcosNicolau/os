#include<memory_comms.h>

int fd_memory;

void send_create_process(t_pcb *pcb){
    t_packet* packet = packet_new(CREATE_PROCESS);
    packet_addUInt32(packet,pcb->context->pid);
    packet_addString(packet,pcb->text_path);
    packet_send(packet,fd_memory);
    packet_free(packet);
}

bool recv_create_process(t_log* logger){
    t_packet* packet = packet_new(-1);
    if(packet_recv(fd_memory,packet)==-1){
        log_error(logger,"Error desconexion de memoria");
        packet_free(packet);
        exit(1);
    }
    bool res = packet->op_code == ERROR_CREATING_PROCESS;
    packet_free(packet);
    return res;
}

void send_end_process(uint32_t pid){
    t_packet* packet = packet_new(END_PROCESS);
    packet_addUInt32(packet,pid);
    packet_send(packet,fd_memory);
    packet_free(packet);
}

void recv_end_process(t_log* logger){
    t_packet* packet = packet_new(-1);
    if(packet_recv(fd_memory,packet)==-1){
        log_error(logger,"Error desconexion de memoria");
        packet_free(packet);
        exit(1);
    }
    packet_free(packet);
}