#include<interrupt_comms.h>

int fd_interrupt;

void send_interrupt(uint8_t reason){
    t_packet* packet = packet_new(reason);
    packet_send(packet,fd_interrupt);
    packet_free(packet);
}