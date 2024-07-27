#include<utils.h>

char *generate_string_of_pids(t_sync_queue *queue)
{
    char *pids = strdup("[");
    void add_pid(void *elem)
    {
        t_pcb *pcb = (t_pcb *)elem;
        string_append_with_format(&pids, "%d,", pcb->context->pid);
    }
    sync_queue_iterate(queue, add_pid);
    if (strlen(pids) > 1)
        pids[strlen(pids) - 1] = ']';
    else
        string_append(&pids, "]");
    return pids;
}
