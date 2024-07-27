#include<dispatch.h>

void send_dispatch_reason(t_opcode reason, t_exec_context* contxt){
    t_packet* packet = packet_new(reason);
    packet_add_context(packet,contxt);
    packet_send(packet,cli_dispatch_fd);
    packet_free(packet);
}

int wait_for_context(t_exec_context* contxt){
    t_packet* packet = packet_new(-1);
    if(packet_recv(cli_dispatch_fd,packet)==-1){
        packet_free(packet);
        return -1;
    }
    
    packet_get_context(packet->buffer,contxt);
    packet_free(packet);
    return 0;
}