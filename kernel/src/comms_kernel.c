#include <comms_kernel.h>

void process_conn(void *void_args)
{
    t_process_conn_args *args = (t_process_conn_args *)void_args;
    t_log *logger = args->logger;
    int client_fd = args->fd;
    free(args);
    t_blocked_queue* this_blocked_queue = NULL;
    t_interface* this_interface = NULL;
    
    while (client_fd != -1)
    {
        t_packet *packet = packet_new(0);
        if (packet_recv(client_fd, packet) == -1) // SE DESCONECTA ENTONCES
        {
            // t_sync_queue* blocked_queue = get_blocked_queue_by_fd(client_fd);
            //  deberia mandar todos a exit pero por ahora...
            // t_interface *interface = interface_get_by_fd(client_fd);
            if (this_interface)
            {
                log_warning(logger, "Interfaz %s de tipo %s se desconecto, todos los bloqueados por ella iran a exit", this_interface->name, this_interface->type);
                interface_destroy(this_interface);
            }
            else
                log_debug(logger, "no se conecto de una");
            handle_pause();
            void iterator(void *elem)
            {
                t_pcb *pcb = (t_pcb *)elem;
                log_info(logger, "Finaliza el proceso %d - Motivo: Error desconexion de interfaz", pcb->context->pid);
                move_pcb_to_exit(pcb, logger);
            }
            // mando todos los procesos de esa cola a exit y elimino la cola mas la interfaz
            if (this_blocked_queue)
            {
                sync_queue_iterate(this_blocked_queue->block_queue,iterator);
                sync_queue_clean(this_blocked_queue->block_queue);
                remove_and_destroy_blocked_queue(this_blocked_queue);
            }
            packet_free(packet);
            return;
        }
        switch (packet->op_code)
        {
        case NEW_INTERFACE:
        {
            this_interface = malloc(sizeof(t_interface));
            this_interface->msg_queue = sync_queue_create();
            this_interface->fd = client_fd;
            this_interface->name = packet_getString(packet->buffer);
            this_interface->type = packet_getString(packet->buffer);
            interface_add(this_interface);
            this_blocked_queue = add_blocked_queue(this_interface->name, client_fd);
            log_info(logger, "New interface registered: name: %s - type: %s", this_interface->name, this_interface->type);
            break;
        }
        case IO_DONE:
        {
            t_interface_io_done_msg *msg = malloc(sizeof(t_interface_io_done_msg));
            interface_decode_io_done(packet->buffer, msg);
            log_debug(logger, "Interface %s requested by pid %d done", msg->interface_name, msg->pid);
            handle_pause();
            scheduler.block_to_ready(this_blocked_queue, logger);

            t_packet* packet = queue_sync_pop(this_interface->msg_queue);
            packet_free(packet);
            if (sync_queue_length(this_interface->msg_queue) > 0) {
                packet = sync_queue_peek(this_interface->msg_queue,0);
                packet_send(packet,this_interface->fd);
            }
            // print_ready_queue(logger);
            interface_destroy_io_done(msg);
            break;
        }
        case IO_ERROR: {
            handle_pause();
            t_packet* packet = queue_sync_pop(this_interface->msg_queue);
            packet_free(packet);
            if (sync_queue_length(this_interface->msg_queue) > 0) {
                packet = sync_queue_peek(this_interface->msg_queue,0);
                packet_send(packet,this_interface->fd);
            }
            block_to_exit(this_blocked_queue, logger);
            break;
        }
        case -1:
            log_error(logger, "client disconnect");
            packet_free(packet);
            return;

        default:
            log_error(logger, "undefined behaviour with opcode: %d", packet->op_code);
            break;
        }
        packet_free(packet);
    }
}

void handle_connections(void *void_args)
{
    t_process_conn_args *args = (t_process_conn_args *)void_args;
    socket_acceptOnDemand(args->fd, args->logger, process_conn);
}