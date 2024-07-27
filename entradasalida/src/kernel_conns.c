#include <kernel_conns.h>

static char *io_op_to_string(t_opcode operation);

static char *string_from_read_mem_msg(t_memory_read_ok_msg *msg, size_t size)
{
    char *aux = malloc(size + 1);
    memset(aux, 0x0, size + 1);
    memcpy(aux, msg->value, size);
    return aux;
}

int registerResourceInKernel(int kernel_fd, t_log *logger, t_io_config *cfg_io)
{
    log_info(logger, "Registering i/o interface %s", cfg_io->name);
    t_packet *packet = packet_new(NEW_INTERFACE);
    packet_addString(packet, cfg_io->name);
    packet_addString(packet, cfg_io->tipo_interfaz);
    int res = packet_send(packet, kernel_fd);
    packet_free(packet);

    return res;
}

static bool invalid_read_write(t_fcb* fcb, uint32_t size, uint32_t file_pointer){
    int last_byte_requested = size + file_pointer;
    return fcb->size < last_byte_requested;
}

void handleKernelIncomingMessage(uint8_t client_fd, uint8_t operation, t_buffer *buffer, void *_args)
{
    struct kernel_incoming_message_args *args = (struct kernel_incoming_message_args *)_args;
    int kernel_fd = args->kernel_fd;
    int memory_fd = args->memory_fd;
    t_log *logger = args->logger;
    t_io_config *config = args->config;
    // not freeing args coz they point to the same address in all requests

    uint32_t pid = packet_getUInt32(buffer);
    log_info(logger, "PID: %d - Operacion: %s", pid, io_op_to_string(operation));

    void send_done(void)
    {
        interface_send_io_done(kernel_fd, interface_name, pid);
    }

    void send_error(void)
    {
        interface_send_io_error(kernel_fd, interface_name, pid, IO_ERROR);
    }

    void do_work(int work)
    {
        msleep(config->unidad_trabajo * work);
    }

    switch (operation)
    {
    case IO_GEN_SLEEP:
    {
        t_interface_io_gen_sleep_msg *msg = malloc(sizeof(t_interface_io_gen_sleep_msg));
        interface_decode_io_gen_sleep(buffer, msg);

        do_work(msg->work_units);
        send_done();

        interface_destroy_io_gen_sleep(msg);
        break;
    }
    case IO_STDIN_READ:
    {
        t_interface_io_stdin_read_msg *msg = malloc(sizeof(t_interface_io_stdin_read_msg));
        interface_decode_io_stdin_read(buffer, msg);

        // ask for prompt
        char *str = prompt(msg->size);

        memory_send_write(memory_fd, pid, msg->access_list, msg->size, str);

        t_packet *res = packet_new(-1);
        if (packet_recv(memory_fd, res) == -1)
        {
            log_error(logger, "Error, desconexion de memoria");
            packet_free(res);
            break;
        }

        if (res->op_code != WRITE_MEM_OK)
        {
            log_info(logger, "Error al escribir en memoria");
        }

        log_info(logger, "MEMORY SUCCESSFULLY SAVED");

        send_done();

        interface_destroy_io_stdin_read(msg);
        free(str);
        packet_free(res);

        break;
    }
    case IO_STDOUT_WRITE:
    {
        t_interface_io_stdout_write_msg *msg = malloc(sizeof(t_interface_io_stdout_write_msg));
        interface_decode_io_stdout_write(buffer, msg);

        memory_send_read(memory_fd, pid, msg->access_list, msg->size);

        t_packet *res = packet_new(-1);
        if (packet_recv(memory_fd, res) == -1)
        {
            log_error(logger, "Error al leer de memoria");
            packet_free(res);
            break;
        }

        if (res->op_code == READ_MEM_OK)
        {
            t_memory_read_ok_msg *ok_msg = malloc(sizeof(t_memory_read_ok_msg));
            memory_decode_read_ok(res->buffer, ok_msg, msg->size);
            char *result = string_from_read_mem_msg(ok_msg, msg->size);
            log_info(logger, "MEMORY VALUE: %s", result);
            free(result);
            memory_destroy_read_ok(ok_msg);
        }

        send_done();
        packet_free(res);
        interface_destroy_io_stdout_write(msg);

        break;
    }
    case IO_FS_CREATE:
    {
        t_interface_io_dialfs_create_msg *msg = malloc(sizeof(t_interface_io_dialfs_create_msg));
        interface_decode_io_dialfs_create(buffer, msg);
        do_work(1);
        if (file_already_exists(msg->file_name))
        {
            log_warning(logger, "PID %d - No hace nada porque el archivo %s ya fue creado", pid, msg->file_name);
            send_done();
            interface_destroy_io_dialfs_create(msg);
            break;
        }

        create_file(msg->file_name);

        log_info(logger, "PID: %d - Crear Archivo: %s", pid, msg->file_name);

        send_done();

        interface_destroy_io_dialfs_create(msg);
        break;
    }
    case IO_FS_DELETE:
    {
        t_interface_io_dialfs_del_msg *msg = malloc(sizeof(t_interface_io_dialfs_del_msg));
        interface_decode_io_dialfs_del(buffer, msg);
        do_work(1);
        if (!file_already_exists(msg->file_name))
        {
            log_warning(logger, "PID %d - El archivo %s no existe", pid, msg->file_name);
            send_done();
            interface_destroy_io_dialfs_del(msg);
            break;
        }

        delete_file(msg->file_name);
        log_info(logger, "PID: %d - Eliminar Archivo: %s", pid, msg->file_name);

        send_done();

        interface_destroy_io_dialfs_del(msg);
        break;
    }
    case IO_FS_TRUNCATE:
    {
        do_work(1);
        t_interface_io_dialfs_truncate_msg *msg = malloc(sizeof(t_interface_io_dialfs_truncate_msg));
        interface_decode_io_dialfs_truncate(buffer, msg);
        if (!file_already_exists(msg->file_name))
        {
            log_warning(logger, "PID %d - El archivo %s no existe", pid, msg->file_name);
            send_done();
            interface_destroy_io_dialfs_truncate(msg);
            break;
        }

        uint32_t target_blocks = ceil((double)msg->size / cfg_io->block_size);
        log_debug(logger, "variables: tam recibido %d taget_blocks; %d", msg->size, target_blocks);
        t_fcb *fcb = get_metadata(msg->file_name);
        uint32_t current_blocks = ceil((double)fcb->size / cfg_io->block_size) ? ceil((double)fcb->size / cfg_io->block_size) : 1;
        // if (!current_blocks)
        //     current_blocks = 1;

        if (target_blocks < current_blocks)
        {
            uint32_t first_block_to_remove = fcb->first_block + target_blocks;
            free_blocks(first_block_to_remove, current_blocks - target_blocks);
        }
        else if (target_blocks > current_blocks)
        {
            if ((target_blocks - current_blocks) > total_free_blocks())
            {
                log_error(logger, "No hay suficiente espacio disponible en el disco para ampliar el archivo %s", msg->file_name);
                send_error();
                free(fcb);
                interface_destroy_io_dialfs_truncate(msg);
                break;
            }
            uint32_t free_contiguous_blocks = free_contiguous_blocks_from(fcb->first_block + current_blocks);
            log_debug(logger, "free contigous: %d", free_contiguous_blocks);
            if (target_blocks - current_blocks > free_contiguous_blocks)
            {
                log_info(logger, "PID: %d - Inicio Compactacion", pid);
                compact(msg->file_name, target_blocks);
                msleep(cfg_io->retraso_compactacion);
                log_info(logger, "PID: %d - Fin compactacion", pid);
            }
            else
            {
                occupy_free_blocks(fcb->first_block + current_blocks, target_blocks - current_blocks);
            }
        }

        fcb_set_size(msg->file_name, msg->size);

        log_info(logger, "PID: %d - Truncar Archivo: %s - Tamaño: %d", pid, msg->file_name, msg->size);

        send_done();

        free(fcb);
        interface_destroy_io_dialfs_truncate(msg);
        break;
    }
    case IO_FS_READ:
    {
        do_work(1);
        t_interface_io_dialfs_read_msg *msg = malloc(sizeof(t_interface_io_dialfs_read_msg));
        interface_decode_io_dialfs_read(buffer, msg);

        if (!file_already_exists(msg->file_name))
        {
            log_warning(logger, "PID %d - El archivo %s no existe", pid, msg->file_name);
            send_error();
            interface_destroy_io_dialfs_read(msg);
            break;
        }

        t_fcb *fcb = get_metadata(msg->file_name);

        if(invalid_read_write(fcb,msg->size,msg->file_pointer)){
            log_error(logger,"PID %d - Lectura invalida en archivo %s",pid,msg->file_name);
            exit(1);
        }

        void *value = read_blocks(fcb->first_block, msg->file_pointer, msg->size);

        log_info(logger, "PID: %d - Leer Archivo: %s - Tamaño a Leer: %d - Puntero Archivo: %d", pid, msg->file_name, msg->size, msg->file_pointer);

        memory_send_write(memory_fd, pid, msg->access_list, msg->size, value);

        t_packet *res = packet_new(-1);
        if (packet_recv(memory_fd, res) == -1)
        {
            log_error(logger, "Error, desconexion de memoria");
            packet_free(res);
            break;
        }

        if (res->op_code != WRITE_MEM_OK)
        {
            log_info(logger, "Error al escribir en memoria");
        }

        send_done();

        free(value);
        packet_free(res);
        interface_destroy_io_dialfs_read(msg);
        free(fcb);
        break;
    }
    case IO_FS_WRITE:
    {
        do_work(1);
        t_interface_io_dialfs_write_msg *msg = malloc(sizeof(t_interface_io_dialfs_write_msg));
        interface_decode_io_dialfs_write(buffer, msg);

        if (!file_already_exists(msg->file_name))
        {
            log_warning(logger, "PID %d - El archivo %s no existe", pid, msg->file_name);
            send_error();
            interface_destroy_io_dialfs_write(msg);
            break;
        }
        t_fcb *fcb = get_metadata(msg->file_name);

        if(invalid_read_write(fcb,msg->size,msg->file_pointer)){
            log_error(logger,"PID %d - Escritura invalida en archivo %s",pid,msg->file_name);
            exit(1);
        }

        memory_send_read(memory_fd, pid, msg->access_list, msg->size);

        t_packet *res = packet_new(-1);

        if (packet_recv(memory_fd, res) == -1)
        {
            log_error(logger, "SE JODIO LA MEMORIA");
            packet_free(res);
            break;
        }

        t_memory_read_ok_msg *ok_msg = malloc(sizeof(t_memory_read_ok_msg));
        memory_decode_read_ok(res->buffer, ok_msg, msg->size);


        write_blocks(fcb->first_block, msg->file_pointer, ok_msg->value, msg->size);

        memory_destroy_read_ok(ok_msg);

        log_info(logger, "PID: %d - Escribir a Archivo: %s - Tamaño a Escribir: %d - Puntero Archivo: %d", pid, msg->file_name, msg->size, msg->file_pointer);

        send_done();

        interface_destroy_io_dialfs_write(msg);
        free(fcb);
        packet_free(res);
        break;
    }
    default:
    {
        log_error(logger, "undefined behaviour with opcode: %d", operation);
        break;
    }
    }
}

static char *io_op_to_string(t_opcode operation)
{
    switch (operation)
    {
    case IO_GEN_SLEEP:
        return "IO_GEN_SLEEP";
    case IO_STDIN_READ:
        return "IO_STDIN_READ";
    case IO_STDOUT_WRITE:
        return "IO_STDOUT_WRITE";
    case IO_FS_CREATE:
        return "IO_FS_CREATE";
    case IO_FS_DELETE:
        return "IO_FS_DELETE";
    case IO_FS_TRUNCATE:
        return "IO_FS_TRUNCATE";
    case IO_FS_READ:
        return "IO_FS_READ";
    case IO_FS_WRITE:
        return "IO_FS_WRITE";
    default:
        return "";
    }
}
