#include <pcb.h>

uint32_t get_next_pid(void);
void init_context(t_exec_context *context);

t_pcb *pcb_create(char *path)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));
    pcb->state = NEW;
    pcb->context = malloc(sizeof(t_exec_context));
    pcb->context->pid = get_next_pid();
    init_context(pcb->context);
    // asumo que si es por fifo me da cero y listo lo ignoro
    pcb->context->quantum = cfg_kernel->quantum;
    pcb->text_path = strdup(path);

    pcb->taken_resources = dictionary_create();
    for(int i = 0 ; cfg_kernel->recursos[i]!=NULL; i++){
        int * instances = malloc(sizeof(int));
        *instances = 0;
        dictionary_put(pcb->taken_resources,cfg_kernel->recursos[i],instances);
    }
    pcb->sigterm = false;
    return pcb;
}

void pcb_destroy(t_pcb *pcb)
{
    free(pcb->context);
    free(pcb->text_path);
    dictionary_destroy_and_destroy_elements(pcb->taken_resources,free);
    free(pcb);
}

void init_context(t_exec_context *context)
{
    t_cpu_registers reg = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    context->registers = reg;
}

uint32_t get_next_pid(void)
{
    static uint32_t pid_counter = 0;
    return ++pid_counter;
}

char *pcb_state_to_string(t_pcb *pcb)
{
    if (!pcb)
        return "";
    switch (pcb->state)
    {
    case NEW:
        return "NEW";
    case READY:
        return "READY";
    case BLOCKED:
        return "BLOCKED";
    case EXEC:
        return "EXEC";
    case EXIT:
        return "EXIT";
    default:
        return "";
    }
}