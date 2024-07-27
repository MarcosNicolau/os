#include <instr_set.h>

int current_exec_process_has_finished = 0;

void set(char **args, t_log *logger);
void sum(char **args, t_log *logger);
void sub(char **args, t_log *logger);
void jnz(char **args, t_log *logger);
void io_gen_sleep(char **args, t_log *logger);
void mov_in(char **args, t_log *logger);
void mov_out(char **args, t_log *logger);
void resize(char **args, t_log *logger);
void copy_string(char **args, t_log *logger);
void wait_instr(char **args, t_log *logger);
void signal_instr(char **args, t_log *logger); // cambio el nombre por confilcto con el signal para el ctrl+C
void io_stdin_read(char **args, t_log *logger);
void io_stdout_write(char **args, t_log *logger);
void io_fs_create(char **args, t_log *logger);
void io_fs_delete(char **args, t_log *logger);
void io_fs_truncate(char **args, t_log *logger);
void io_fs_write(char **args, t_log *logger);
void io_fs_read(char **args, t_log *logger);
// para no tener conflitcto
void instruction_exit(char **args, t_log *logger);

static void send_io_gen_sleep(char *interface_name, int work_units);
enum StdType
{
    IN,
    OUT
};
static void send_io_std(enum StdType type, char *interface_name, t_list *access_list, uint32_t size);
static void send_wait_resource(char *resource_name);
static void send_signal_resource(char *resource_name);
static void send_resize(uint32_t size);
static void send_dial_fs_create(char* interface_name, char* file_name);
static void send_dial_fs_delete(char* interface_name, char* file_name);
static void send_dial_fs_truncate(char* interface_name, char* file_name,uint32_t size);
static void send_dial_fs_read_write(char* interface_name, char* file_name,t_list* access_list,uint32_t size,uint32_t file_pointer,bool is_write);

t_instruction INSTRUCTION_SET[] = {{"SET", set}, {"SUM", sum}, {"SUB", sub}, {"JNZ", jnz}, {"IO_GEN_SLEEP", io_gen_sleep}, {"MOV_IN", mov_in}, {"MOV_OUT", mov_out}, {"RESIZE", resize}, {"COPY_STRING", copy_string}, {"WAIT", wait_instr}, {"SIGNAL", signal_instr}, {"IO_STDIN_READ", io_stdin_read}, {"IO_STDOUT_WRITE", io_stdout_write}, {"IO_FS_CREATE", io_fs_create}, {"IO_FS_DELETE", io_fs_delete}, {"IO_FS_TRUNCATE", io_fs_truncate}, {"IO_FS_WRITE", io_fs_write}, {"IO_FS_READ", io_fs_read}, {"EXIT", instruction_exit}, {NULL, NULL}};

// CON ESTA NOMENCLATURA CADA INSTRUCCION DEBE DESERIALIZAR SUS PARAMETROSs

t_instruction *instruction_get_by_name(char *name)
{
    for (uint8_t i = 0; INSTRUCTION_SET[i].name != NULL; i++)
    {
        if (!strcmp(INSTRUCTION_SET[i].name, name))
            return &INSTRUCTION_SET[i];
    }
    return NULL;
}

void set(char **args, t_log *logger)
{
    t_register *reg = register_get_by_name(args[0]);

    if (sizeof(uint8_t) == reg->size)
    {
        uint8_t value = (uint8_t)atoi(args[1]);
        memcpy(reg->address, &value, reg->size);
        return;
    }

    uint32_t value = (uint32_t)atoi(args[1]);
    memcpy(reg->address, &value, reg->size);
}

void sum(char **args, t_log *logger)
{
    t_register *dest = register_get_by_name(args[0]);
    t_register *src = register_get_by_name(args[1]);

    if (sizeof(uint8_t) == dest->size)
    {
        uint8_t src_value = register_get_value(src);
        uint8_t *dest_value = (uint8_t *)dest->address;

        *dest_value += src_value;
        return;
    }

    uint32_t src_value = register_get_value(src);
    uint32_t *dest_value = (uint32_t *)dest->address;

    *dest_value += src_value;
}

void sub(char **args, t_log *logger)
{
    t_register *dest = register_get_by_name(args[0]);
    t_register *src = register_get_by_name(args[1]);

    if (sizeof(uint8_t) == dest->size)
    {
        uint8_t src_value = register_get_value(src);
        uint8_t *dest_value = (uint8_t *)dest->address;

        *dest_value -= src_value;
        return;
    }

    uint32_t src_value = register_get_value(src);
    uint32_t *dest_value = (uint32_t *)dest->address;

    *dest_value -= src_value;
}

void jnz(char **args, t_log *logger)
{
    uint32_t *pc = &context.registers.pc;
    t_register *reg = register_get_by_name(args[0]);
    if (sizeof(uint8_t) == reg->size)
    {
        uint8_t *value = (uint8_t *)reg->address;
        *pc = *value ? atoi(args[1]) : *pc;
        return;
    }

    uint32_t *value = (uint32_t *)reg->address;
    *pc = *value ? atoi(args[1]) : *pc;
}

void mov_in(char **args, t_log *logger)
{
    t_register *data = register_get_by_name(args[0]);
    t_register *dir = register_get_by_name(args[1]);

    uint32_t dir_value = register_get_value(dir);

    t_list *access_to_memory = access_to_memory_create(dir_value, data->size, PAGE_SIZE, logger);

    memory_send_read(fd_memory, context.pid, access_to_memory, data->size);
    t_packet *packet = packet_new(-1);
    if (packet_recv(fd_memory, packet) == -1)
    {
        log_error(logger, "Error, desconexion de memoria");
        packet_free(packet);
        return;
    }

    if (packet->op_code != READ_MEM_OK)
    {
        log_info(logger, "Error al leer de memoria");
        packet_free(packet);
        return;
    }

    t_memory_read_ok_msg *ok_msg = malloc(sizeof(t_memory_read_ok_msg));
    memory_decode_read_ok(packet->buffer, ok_msg, data->size);
    memcpy(data->address, ok_msg->value, data->size);

    int offset = 0;
    void iterator(void *elem)
    {
        t_access_to_memory *access = (t_access_to_memory *)elem;
        int temp = 0;
        memcpy(&temp, ok_msg->value + offset, access->bytes_to_access);
        offset += access->bytes_to_access;
        log_info(logger, "PID: %d - Accion: LEER - Direccion Fisica: %d - Valor: %d", context.pid, access->address, temp);
    }
    // aiuda
    list_iterate(access_to_memory, iterator);

    list_destroy_and_destroy_elements(access_to_memory, free);

    memory_destroy_read_ok(ok_msg);
    packet_free(packet);
}

void mov_out(char **args, t_log *logger)
{
    t_register *dir = register_get_by_name(args[0]);
    t_register *data = register_get_by_name(args[1]);

    uint32_t dir_value = register_get_value(dir);

    t_list *access_to_memory = access_to_memory_create(dir_value, data->size, PAGE_SIZE, logger);

    int offset = 0;
    void iterator(void *elem)
    {
        t_access_to_memory *access = (t_access_to_memory *)elem;
        int temp = 0;
        memcpy(&temp, data->address + offset, access->bytes_to_access);
        offset += access->bytes_to_access;
        log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion Fisica: %d - Valor: %d", context.pid, access->address, temp);
    }

    list_iterate(access_to_memory, iterator);

    memory_send_write(fd_memory, context.pid, access_to_memory, data->size, data->address);

    list_destroy_and_destroy_elements(access_to_memory, free);

    t_packet *packet = packet_new(-1);
    if (packet_recv(fd_memory, packet) == -1)
    {
        log_error(logger, "Error, se desconecto la memoria");
        packet_free(packet);
        exit(1);
    }

    if (packet->op_code != WRITE_MEM_OK)
    {
        log_info(logger, "Error al escribir en memoria");
        // hacer algo?? no se
        packet_free(packet);
        return;
    }

    packet_free(packet);
}

void resize(char **args, t_log *logger)
{
    uint32_t size = (uint32_t)atoi(args[0]);
    send_resize(size);
    t_packet *packet = packet_new(-1);
    if (packet_recv(fd_memory, packet) == -1)
    {
        log_error(logger, "error al recibir de memoria");
        packet_free(packet);
        exit(1);
    }
    if (packet->op_code == OUT_OF_MEMORY)
    {
        send_dispatch_reason(OUT_OF_MEMORY, &context);
        wait_for_context(&context);
        clear_interrupt();
    }
    if(packet->op_code == R_SHRINK_OK){
        cleanup_tlb(context.pid,size);
        tlb_dump(logger);
    }

    packet_free(packet);
}

void copy_string(char **args, t_log *logger)
{
    uint32_t size = (uint32_t)atoi(args[0]);
   
    t_list *access_to_memory = access_to_memory_create(context.registers.si, size, PAGE_SIZE, logger);

    memory_send_read(fd_memory, context.pid, access_to_memory, size);
    t_packet *packet = packet_new(-1);
    if (packet_recv(fd_memory, packet) == -1)
    {
        log_error(logger, "Error, desconexion de memoria");
        packet_free(packet);
        return;
    }

    if (packet->op_code != READ_MEM_OK)
    {
        log_info(logger, "Error al leer de memoria");
        packet_free(packet);
        return;
    }


    t_memory_read_ok_msg *ok_msg = malloc(sizeof(t_memory_read_ok_msg));
    memory_decode_read_ok(packet->buffer, ok_msg, size);

    packet_free(packet);
    
    int offset = 0;
    void iterator_read(void *elem)
    {
        t_access_to_memory *access = (t_access_to_memory *)elem;
        char* temp = calloc(access->bytes_to_access+1,1);
        memcpy(temp, ok_msg->value + offset, access->bytes_to_access);
        offset += access->bytes_to_access;
        log_info(logger, "PID: %d - Accion: LEER - Direccion Fisica: %d - Valor: %s", context.pid, access->address, temp);
        free(temp);
    }
    // aiuda
    list_iterate(access_to_memory, iterator_read);

    list_destroy_and_destroy_elements(access_to_memory, free);

    access_to_memory = access_to_memory_create(context.registers.di, size, PAGE_SIZE, logger);

    offset = 0;
    void iterator_write(void *elem)
    {
        t_access_to_memory *access = (t_access_to_memory *)elem;
        char* temp = calloc(access->bytes_to_access+1,1);
        memcpy(temp, ok_msg->value + offset, access->bytes_to_access);
        offset += access->bytes_to_access;
        log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion Fisica: %d - Valor: %s", context.pid, access->address, temp);
        free(temp);
    }

    list_iterate(access_to_memory, iterator_write);

    memory_send_write(fd_memory, context.pid, access_to_memory, size, ok_msg->value);

    list_destroy_and_destroy_elements(access_to_memory, free);

    memory_destroy_read_ok(ok_msg);

    packet = packet_new(-1);
    if (packet_recv(fd_memory, packet) == -1)
    {
        log_error(logger, "Error, se desconecto la memoria");
        packet_free(packet);
        exit(1);
    }

    if (packet->op_code != WRITE_MEM_OK)
    {
        log_info(logger, "Error al escribir en memoria");
        // hacer algo?? no se
        packet_free(packet);
        return;
    }

    packet_free(packet);
}

void wait_instr(char **args, t_log *logger)
{
    // clear_interrupt();
    char *resource_name = args[0];
    int last_pid = context.pid;
    send_wait_resource(resource_name);
    wait_for_context(&context);
    if (context.pid != last_pid)
        clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);
}
void signal_instr(char **args, t_log *logger)
{
    char *resource_name = args[0];
    send_signal_resource(resource_name);
    wait_for_context(&context);
    log_debug(logger, "me llego: pid: %d, quantum: %d", context.pid, context.quantum);
}

void io_gen_sleep(char **args, t_log *logger)
{
    char *interface_name = args[0];
    uint32_t work_units = atoi(args[1]);
    send_io_gen_sleep(interface_name, work_units);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);
}

void io_stdin_read(char **args, t_log *logger)
{
    char *interface_name = args[0];
    t_register *virtual_address = register_get_by_name(args[1]);
    t_register *size_reg = register_get_by_name(args[2]);
    uint32_t size = register_get_value(size_reg);

    uint32_t dir_value = register_get_value(virtual_address);

    t_list *access_to_memory = access_to_memory_create(dir_value, size, PAGE_SIZE, logger);

    send_io_std(IN, interface_name, access_to_memory, size);
    list_destroy_and_destroy_elements(access_to_memory, free);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);
}

void io_stdout_write(char **args, t_log *logger)
{
    char *interface_name = args[0];
    t_register *virtual_address = register_get_by_name(args[1]);
    t_register *size_reg = register_get_by_name(args[2]);
    uint32_t size = register_get_value(size_reg);

    uint32_t dir_value = register_get_value(virtual_address);

    t_list *access_to_memory = access_to_memory_create(dir_value, size, PAGE_SIZE, logger);

    send_io_std(OUT, interface_name, access_to_memory, size);
    list_destroy_and_destroy_elements(access_to_memory, free);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);
}

void io_fs_create(char **args, t_log *logger)
{
    char* interface_name = args[0];
    char* file_name = args[1];
    send_dial_fs_create(interface_name, file_name);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);

}
void io_fs_delete(char **args, t_log *logger)
{
    char* interface_name = args[0];
    char* file_name = args[1];
    send_dial_fs_delete(interface_name, file_name);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);
}

void io_fs_truncate(char **args, t_log *logger)
{
    char* interface_name = args[0];
    char* file_name = args[1];
    t_register *size_reg = register_get_by_name(args[2]);
    uint32_t size = register_get_value(size_reg);
    send_dial_fs_truncate(interface_name,file_name,size);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);
}

void io_fs_write(char **args, t_log *logger)
{
    char* interface_name = args[0];
    char* file_name = args[1];
    t_register *dir_reg = register_get_by_name(args[2]);
    t_register *size_reg = register_get_by_name(args[3]);
    t_register *pointer_reg = register_get_by_name(args[4]);

    uint32_t size = register_get_value(size_reg);

    uint32_t dir_value = register_get_value(dir_reg);

    uint32_t file_pointer = register_get_value(pointer_reg);

    t_list *access_to_memory = access_to_memory_create(dir_value, size, PAGE_SIZE, logger);

    send_dial_fs_read_write(interface_name, file_name, access_to_memory, size, file_pointer, true);  

    list_destroy_and_destroy_elements(access_to_memory, free);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);  

}
void io_fs_read(char **args, t_log *logger)
{
    char* interface_name = args[0];
    char* file_name = args[1];
    t_register *dir_reg = register_get_by_name(args[2]);
    t_register *size_reg = register_get_by_name(args[3]);
    t_register *pointer_reg = register_get_by_name(args[4]);

    uint32_t size = register_get_value(size_reg);

    uint32_t dir_value = register_get_value(dir_reg);

    uint32_t file_pointer = register_get_value(pointer_reg);

    t_list *access_to_memory = access_to_memory_create(dir_value, size, PAGE_SIZE, logger);

    send_dial_fs_read_write(interface_name, file_name, access_to_memory, size, file_pointer, false);  

    list_destroy_and_destroy_elements(access_to_memory, free);
    wait_for_context(&context);
    clear_interrupt();
    log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);  
}
// para no tener conflitcto
void instruction_exit(char **args, t_log *logger)
{
    clear_interrupt();
    send_dispatch_reason(END_PROCESS, &context);
    current_exec_process_has_finished = 1;
}

// MEM PROTOCOL

static void send_resize(uint32_t size)
{
    t_packet *packet = packet_new(RESIZE_PROCESS);
    packet_addUInt32(packet, context.pid);
    packet_addUInt32(packet, size);
    packet_send(packet, fd_memory);
    packet_free(packet);
}

// WAIT and SIGNAL protocol
static void send_wait_resource(char *resource_name)
{
    t_packet *packet = packet_new(WAIT);
    packet_add_context(packet, &context);
    packet_addString(packet, resource_name);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}

static void send_signal_resource(char *resource_name)
{
    t_packet *packet = packet_new(SIGNAL);
    packet_add_context(packet, &context);
    packet_addString(packet, resource_name);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}

// IO protocol
static void send_io_gen_sleep(char *interface_name, int work_units)
{
    t_packet *packet = packet_new(IO_GEN_SLEEP);
    packet_add_context(packet, &context);
    packet_addString(packet, interface_name);
    packet_addUInt32(packet, work_units);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}

static void send_io_std(enum StdType type, char *interface_name, t_list *access_list, uint32_t size)
{
    t_packet *packet = packet_new(type == IN ? IO_STDIN_READ : IO_STDOUT_WRITE);
    packet_add_context(packet, &context);
    packet_addString(packet, interface_name);
    packet_add_list(packet, access_list, (void *)packet_add_access_to_mem);
    packet_addUInt32(packet, size);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}

static void send_dial_fs_create(char* interface_name, char* file_name){
    t_packet* packet = packet_new(IO_FS_CREATE);
    packet_add_context(packet, &context);
    packet_addString(packet,interface_name);
    packet_addString(packet,file_name);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}

static void send_dial_fs_delete(char* interface_name, char* file_name){
    t_packet* packet = packet_new(IO_FS_DELETE);
    packet_add_context(packet, &context);
    packet_addString(packet,interface_name);
    packet_addString(packet,file_name);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}

static void send_dial_fs_truncate(char* interface_name, char* file_name,uint32_t size){
    t_packet* packet = packet_new(IO_FS_TRUNCATE);
    packet_add_context(packet, &context);
    packet_addString(packet,interface_name);
    packet_addString(packet,file_name);
    packet_addUInt32(packet, size);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}

static void send_dial_fs_read_write(char* interface_name, char* file_name,t_list* access_list,uint32_t size,uint32_t file_pointer,bool is_write){
    t_opcode op_code = is_write ? IO_FS_WRITE : IO_FS_READ;
    t_packet* packet = packet_new(op_code);
    packet_add_context(packet, &context);
    packet_addString(packet,interface_name);
    packet_addString(packet,file_name);
    packet_add_list(packet, access_list, (void *)packet_add_access_to_mem);
    packet_addUInt32(packet, size);
    packet_addUInt32(packet, file_pointer);
    packet_send(packet, cli_dispatch_fd);
    packet_free(packet);
}