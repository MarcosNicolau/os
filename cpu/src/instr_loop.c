#include <instr_loop.h>

static char *args_as_string(char **args);
void execute(void (*instr)(char **args, t_log *logger), char **args, t_log *logger);

char *fetch(int fd_memoria, t_log *logger)
{
    log_info(logger, "PID: %d - FETCH - Program Counter: %d", context.pid, context.registers.pc);
    t_packet *request = packet_new(NEXT_INSTRUCTION);
    packet_addUInt32(request, context.pid);
    packet_addUInt32(request, context.registers.pc);
    packet_send(request, fd_memoria);
    packet_free(request);
    request = packet_new(-1);
    if (packet_recv(fd_memoria, request) == -1)
    {
        log_error(logger, "no me llego nada");
        packet_free(request);
        exit(1);
    }
    if (request->op_code != NEXT_INSTRUCTION)
    {
        log_error(logger, "No hay proxima instruccion");
        packet_free(request);
        send_dispatch_reason(NO_INSTRUCTION,&context);
        return NULL;
    }

    char *aux = packet_getString(request->buffer);
    packet_free(request);
    return aux;
}

void decode_and_execute(char *instruction, t_log *logger)
{
    char **tokens = string_split(instruction, " ");
    char *instr_name = tokens[0];
    char **args = tokens + 1;
    char *string_of_args = args_as_string(args);
    t_instruction *inst = instruction_get_by_name(instr_name);
    log_info(logger, "PID: %d - Ejecutando: %s - %s", context.pid, instr_name, string_of_args);
    if(!inst){
        log_error(logger,"INSTRUCCION INVALIDA, NO EJECUTO NADA");
    }
    else
        execute(inst->instr, args, logger);
    free(string_of_args);
    string_array_destroy(tokens);
    free(instruction);
}

void execute(void (*instr)(char **args, t_log *logger), char **args, t_log *logger)
{
    instr(args, logger);
}

void check_interrupt(t_log *logger)
{
    if (interrupt_flag()) {
        send_dispatch_reason(interrupt_reason,&context);
        log_info(logger, "PID: %d - Fue interrumpido", context.pid);
        if(wait_for_context(&context) == -1){
            log_error(logger, "se desconecto el kernel de dispatch");
            exit(1);
        }
        clear_interrupt();
        log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum,context.registers.ax);
    }
}

static char *args_as_string(char **args)
{
    char *result = string_new();
    void iterator(char *elem)
    {
        string_append_with_format(&result, "%s ", elem);
    }
    string_iterate_lines(args, iterator);
    string_trim_right(&result);
    return result;
}