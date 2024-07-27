#include <dispatcher.h>

int fd_dispatch;
// whether the interruptiion was due to a i/o instruction or not

static void handle_quantum(void){
    if(quantum_interruption_thread){
        pthread_cancel(quantum_interruption_thread);
        if(timer){
            temporal_stop(timer);
            time_elapsed = temporal_gettime(timer);
        }  
    }
}

void send_context_to_cpu(t_exec_context *context)
{
    t_packet *packet = packet_new(EXEC_PROCESS); // puede ser otro no importa
    packet_add_context(packet, context);
    packet_send(packet, fd_dispatch);
    packet_free(packet);
}

t_interface *interface_middleware(t_buffer *buffer, uint8_t instruction_to_run, t_pcb *pcb, t_log *logger);

int wait_for_dispatch_reason(t_pcb *pcb, t_log *logger)
{
    t_packet *packet = packet_new(-1);
    if (packet_recv(fd_dispatch, packet) == -1)
    {
        packet_free(packet);
        return -1;
    }
    
    // en todas le desalojo el contexto
    packet_get_context(packet->buffer, pcb->context);
    switch (packet->op_code)
    {
    case INTERRUPT_EXEC:
    {
        // cuando lo interrumpe por consola es aca
        handle_quantum();
        log_info(logger, "Finaliza el proceso %d- Motivo: INTERRUPTED_BY_USER", pcb->context->pid);
        move_pcb_to_exit(pcb, logger);
        break;
    }
    case END_OF_QUANTUM:
    {
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        log_info(logger, "PID: %d - Desalojado por fin de Quantum", pcb->context->pid);
        scheduler.exec_to_ready(pcb, logger);
        break;
    }
    case END_PROCESS:
    {
        handle_quantum();
        handle_pause();
        log_info(logger, "Finaliza el proceso %d- Motivo: SUCCESS", pcb->context->pid);
        move_pcb_to_exit(pcb, logger);
        break;
    }
    case OUT_OF_MEMORY:
    {
        handle_quantum();
        handle_pause();
        log_info(logger, "Finaliza el proceso %d- Motivo: OUT OF MEMORY", pcb->context->pid);
        move_pcb_to_exit(pcb, logger);
        break;
    }
     case NO_INSTRUCTION:
    {
        // caso en el que no se enceuntre la proxima instruccion en el fetch
        handle_quantum();
        handle_pause();
        log_info(logger, "Finaliza el proceso %d- Motivo: No se encontro siguiente instruccion", pcb->context->pid);
        move_pcb_to_exit(pcb, logger);
        break;
    }
    case WAIT:
    {
        char *resource_name = packet_getString(packet->buffer);
        t_blocked_queue *q = get_blocked_queue_by_name(resource_name);
        if (!q)
        {
            handle_quantum();
            handle_pause();
            log_info(logger, "Finaliza el proceso %d- Motivo: INVALID_RESOURCE: %s", pcb->context->pid, resource_name);
            move_pcb_to_exit(pcb, logger);
            free(resource_name);
            break;
        }
        pthread_mutex_lock(&q->resource_mutex);
        q->instances--;
        pthread_mutex_unlock(&q->resource_mutex);
        if (q->instances < 0)
        {
            handle_quantum();
            handle_pause();
            if(handle_sigterm(pcb,logger)){
                pthread_mutex_lock(&q->resource_mutex);
                q->instances++;
                pthread_mutex_unlock(&q->resource_mutex);
                free(resource_name);
                break;   
            }
            scheduler.move_pcb_to_blocked(pcb, q->resource_name, logger);
            log_info(logger, "PID: %d - Bloqueado por: %s", pcb->context->pid, resource_name);
            free(resource_name);
            break;
        }
        int* taken = dictionary_get(pcb->taken_resources,resource_name);
        (*taken)++;
        log_debug(logger,"tomados por %d : %s->%d",pcb->context->pid,resource_name,*taken);
        log_debug(logger, "Instancias del recurso %s: %d", resource_name, q->instances);
        send_context_to_cpu(pcb->context);
        free(resource_name);
        wait_for_dispatch_reason(pcb, logger);
        break;
    }
    case SIGNAL:
    {
        char *resource_name = packet_getString(packet->buffer);
        t_blocked_queue *q = get_blocked_queue_by_name(resource_name);
        if (!q)
        {
            handle_quantum();
            log_info(logger, "Finaliza el proceso %d- Motivo: INVALID_RESOURCE: %s", pcb->context->pid, resource_name);
            move_pcb_to_exit(pcb, logger);
            free(resource_name);
            break;
        }
        instr_signal(pcb,q,logger);    
        send_context_to_cpu(pcb->context);
        free(resource_name);
        wait_for_dispatch_reason(pcb, logger);
        break;
    }
    case IO_GEN_SLEEP:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_GEN_SLEEP, pcb, logger);
        if (!interface)
            break;
        t_interface_io_gen_sleep_msg *msg = malloc(sizeof(t_interface_io_gen_sleep_msg));
        interface_decode_io_gen_sleep(packet->buffer, msg);
        t_packet* packet = interface_serialize_io_gen_sleep(pcb->context->pid, msg->work_units);
        interface_destroy_io_gen_sleep(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet, interface->fd);
        }
        break;
    }
    case IO_STDIN_READ:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_STDIN_READ, pcb, logger);
        if (!interface)
            break;
        t_interface_io_stdin_read_msg *msg = malloc(sizeof(t_interface_io_stdin_read_msg));
        interface_decode_io_stdin_read(packet->buffer, msg);
        t_packet* packet = interface_serialize_io_stdin_read(pcb->context->pid, msg);
        interface_destroy_io_stdin_read(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet,interface->fd);
        }
        break;
    }
    case IO_STDOUT_WRITE:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_STDOUT_WRITE, pcb, logger);
        if (!interface)
            break;
        t_interface_io_stdout_write_msg *msg = malloc(sizeof(t_interface_io_stdout_write_msg));
        interface_decode_io_stdout_write(packet->buffer, msg);
        t_packet *packet = interface_serialize_io_stdout_write(pcb->context->pid, msg);
        interface_destroy_io_stdout_write(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet, interface->fd);
        }
        break;
    }
    case IO_FS_CREATE:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_FS_CREATE, pcb, logger);
        if (!interface)
            break;
        t_interface_io_dialfs_create_msg *msg = malloc(sizeof(t_interface_io_dialfs_create_msg));
        interface_decode_io_dialfs_create(packet->buffer, msg);
        t_packet *packet = interface_serialize_io_dialfs_create(pcb->context->pid, msg);
        interface_destroy_io_dialfs_create(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet, interface->fd);
        }
        break;
    }
    case IO_FS_DELETE:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_FS_DELETE, pcb, logger);
        if (!interface)
            break;
        t_interface_io_dialfs_del_msg *msg = malloc(sizeof(t_interface_io_dialfs_del_msg));
        interface_decode_io_dialfs_del(packet->buffer, msg);
        t_packet *packet = interface_serialize_io_dialfs_del(pcb->context->pid, msg);
        interface_destroy_io_dialfs_del(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet, interface->fd);
        }
        break;
    }
    case IO_FS_TRUNCATE:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_FS_TRUNCATE, pcb, logger);
        if (!interface)
            break;
        t_interface_io_dialfs_truncate_msg *msg = malloc(sizeof(t_interface_io_dialfs_truncate_msg));
        interface_decode_io_dialfs_truncate(packet->buffer, msg);
        t_packet *packet = interface_serialize_io_dialfs_truncate(pcb->context->pid, msg);
        interface_destroy_io_dialfs_truncate(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet, interface->fd);
        }
        break;
    }
    case IO_FS_READ:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_FS_READ, pcb, logger);
        if (!interface)
            break;
        t_interface_io_dialfs_read_msg *msg = malloc(sizeof(t_interface_io_dialfs_read_msg));
        interface_decode_io_dialfs_read(packet->buffer, msg);
        t_packet *packet = interface_serialize_io_dialfs_read(pcb->context->pid, msg);
        interface_destroy_io_dialfs_read(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet, interface->fd);
        }
        break;
    }
    case IO_FS_WRITE:
    {
        handle_quantum();
        handle_pause();
        if(handle_sigterm(pcb,logger))
            break;
        t_interface *interface = interface_middleware(packet->buffer, IO_FS_WRITE, pcb, logger);
        if (!interface)
            break;
        t_interface_io_dialfs_write_msg *msg = malloc(sizeof(t_interface_io_dialfs_write_msg));
        interface_decode_io_dialfs_write(packet->buffer, msg);
        t_packet *packet = interface_serialize_io_dialfs_write(pcb->context->pid, msg);
        interface_destroy_io_dialfs_write(msg);
        queue_sync_push(interface->msg_queue,packet);
        if(sync_queue_length(interface->msg_queue)==1){
            packet_send(packet, interface->fd);
        }
        break;
    }
    default:
        log_error(logger, "operacion desconocida opcode: %d", packet->op_code);
        break;
    }

    packet_free(packet);
    return 0;
}


/**
 * should be run at the beginning of every dispatch request that correspond to interfaces
 * it handles:
 *  - validation
 *  - process scheduling
 *
 * @returns NULL on error, otherwise the corresponding `t_interface`
 */
t_interface *interface_middleware(t_buffer *buffer, uint8_t instruction_to_run, t_pcb *pcb, t_log *logger)
{
    char *interface_name = packet_getString(buffer);
    t_interface *interface = interface_validate(interface_name, instruction_to_run);
    if (!interface)
    {
        log_info(logger, "Finaliza el proceso %d- Motivo: INVALID_INTERFACE: %s ", pcb->context->pid, interface_name);
        free(interface_name);
        // nunca pase por bloqueado asi que no deberia explotar
        move_pcb_to_exit(pcb, logger);
        return NULL;
    }
    if (scheduler.move_pcb_to_blocked(pcb, interface->name, logger) == -1)
    {
        log_error(logger, "Could not find blocked queue for %s", interface_name);
        free(interface_name);
        move_pcb_to_exit(pcb, logger);
        return NULL;
    }
    log_info(logger, "PID: %d - Bloqueado por: %s", pcb->context->pid, interface_name);
    free(interface_name);
    return interface;
}
