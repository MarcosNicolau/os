#include <fifo.h>

t_pcb *ready_to_exec_fifo(void)
{
    t_pcb *pcb = queue_sync_pop(ready_queue);
    queue_sync_push(exec_queue, pcb);
    pcb->state = EXEC;
    send_context_to_cpu(pcb->context);
    return pcb;
}

void dispatch_fifo(t_pcb *pcb, t_log *logger)
{
    if (wait_for_dispatch_reason(pcb, logger) == -1)
    {
        log_error(logger, "error waiting for cpu context");
    }
}

void block_to_ready_fifo(t_blocked_queue* queue, t_log *logger)
{
    //wait sem cola bloqueado
    t_pcb *pcb = blocked_queue_pop(queue);
    if(!pcb)
        log_error(logger,"blocked queue not found");
    if(is_resource(queue->resource_name)){
        int* taken = dictionary_get(pcb->taken_resources,queue->resource_name);
        (*taken)++;
        log_debug(logger,"tomados por %d : %s->%d",pcb->context->pid,queue->resource_name,*taken);
    }
    if(handle_sigterm(pcb,logger))
        return;
    pcb->state = READY;
    log_info(logger, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", pcb->context->pid);
    queue_sync_push(ready_queue, pcb);
    print_ready_queue(logger, false);
    sem_post(&scheduler.sem_ready);
}

int move_pcb_to_blocked_fifo(t_pcb *pcb, char *resource_name, t_log *logger)
{
    if(blocked_queue_push(resource_name, pcb) == -1)
        return -1;
    pcb->state = BLOCKED;
    log_info(logger, "PID: %d - Estado Anterior: EXEC - Estado Actual: BLOCKED", pcb->context->pid);
    return 0;
}