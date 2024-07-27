#include <state_queues.h>
pthread_mutex_t MUTEX_LISTA_BLOCKEADOS;

t_sync_queue *new_queue;
t_sync_queue *ready_queue;
t_sync_queue *ready_plus_queue;
t_sync_queue *exec_queue;
t_sync_queue *exit_queue;
static t_list *_blocked_queues;

static void destroy_blocked_queues(void);
void pcb_destroyer(void *elem);
void add_resources_to_blocked_queues(void);

void init_queues(void)
{
    new_queue = sync_queue_create();
    ready_queue = sync_queue_create();
    ready_plus_queue = sync_queue_create();
    exec_queue = sync_queue_create();
    exit_queue = sync_queue_create();
    _blocked_queues = list_create();
    add_resources_to_blocked_queues();
    pthread_mutex_init(&MUTEX_LISTA_BLOCKEADOS, NULL);
}

void destroy_queues(void)
{
    sync_queue_destroy_with_destroyer(new_queue, pcb_destroyer);
    sync_queue_destroy_with_destroyer(ready_queue, pcb_destroyer);
    sync_queue_destroy_with_destroyer(ready_plus_queue, pcb_destroyer);
    sync_queue_destroy_with_destroyer(exec_queue, pcb_destroyer);
    sync_queue_destroy_with_destroyer(exit_queue, pcb_destroyer);
    destroy_blocked_queues();
    pthread_mutex_destroy(&MUTEX_LISTA_BLOCKEADOS);
}

t_blocked_queue *blocked_queue_create(char *name, int value)
{
    t_blocked_queue *q = malloc(sizeof(t_blocked_queue));
    q->resource_name = strdup(name);
    q->instances = value;
    sem_init(&q->sem_process_count, 0, 0);
    q->block_queue = sync_queue_create();
    return q;
}

void blocked_queue_destroy(t_blocked_queue *q)
{
    sem_destroy(&q->sem_process_count);
    free(q->resource_name);
    sync_queue_destroy(q->block_queue);
    free(q);
}

void blocked_queue_destroy_and_destroy_elements(t_blocked_queue *q)
{
    sem_destroy(&q->sem_process_count);
    free(q->resource_name);
    sync_queue_destroy_with_destroyer(q->block_queue, pcb_destroyer);
    free(q);
}

void pcb_destroyer(void *elem)
{
    t_pcb *pcb = (t_pcb *)elem;
    pcb_destroy(pcb);
}

t_blocked_queue *add_blocked_queue(char *resource_name, int value)
{
    t_blocked_queue *q = blocked_queue_create(resource_name, value);
    pthread_mutex_lock(&MUTEX_LISTA_BLOCKEADOS);
    list_add(_blocked_queues, q);
    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
    return q;
}

t_blocked_queue *get_blocked_queue_by_name(char *resource_name)
{
    bool closure(void *elem)
    {
        t_blocked_queue *q = (t_blocked_queue *)elem;
        return !strcmp(q->resource_name, resource_name);
    }
    pthread_mutex_lock(&MUTEX_LISTA_BLOCKEADOS);
    t_blocked_queue *target = list_find(_blocked_queues, closure);
    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
    return target;
}

int blocked_queue_push(char *resource_name, void *elem)
{
    t_blocked_queue *queue = get_blocked_queue_by_name(resource_name);
    if (!queue)
        return -1;
    queue_sync_push(queue->block_queue, elem);
    sem_post(&queue->sem_process_count);
    return 0;
}

void *blocked_queue_pop(t_blocked_queue *queue)
{
    sem_wait(&queue->sem_process_count);
    void *elem = queue_sync_pop(queue->block_queue);
    return elem;
}

static void destroy_blocked_queues(void)
{
    void destroyer(void *queue)
    {
        t_blocked_queue *q = (t_blocked_queue *)queue;
        blocked_queue_destroy_and_destroy_elements(q);
    }
    list_destroy_and_destroy_elements(_blocked_queues, destroyer);
}

void blocked_queues_iterate(void (*iterator)(void *))
{ // iterator tiene que recibir una cola
    pthread_mutex_lock(&MUTEX_LISTA_BLOCKEADOS);
    list_iterate(_blocked_queues, iterator);
    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
}

void print_ready_queue(t_log *logger, bool is_ready_plus)
{
    // esto se puede mejorar pero por ahora pasa
    if (is_ready_plus)
    {
        char *pids_plus = generate_string_of_pids(ready_plus_queue);
        log_info(logger, "Cola Ready PLUS: %s", pids_plus);
        free(pids_plus);
    }
    else
    {
        char *pids = generate_string_of_pids(ready_queue);
        log_info(logger, "Cola Ready: %s", pids);
        free(pids);
    }
}

void add_resources_to_blocked_queues(void)
{
    int i = 0;
    void add_resource(char *resource)
    {
        t_blocked_queue *queue = add_blocked_queue(resource, atoi(*(cfg_kernel->instancias_recursos + i)));
        pthread_mutex_init(&queue->resource_mutex, 0);
        i++;
    }
    string_iterate_lines(cfg_kernel->recursos, add_resource);
}

bool is_resource(char *name)
{
    for (int i = 0; cfg_kernel->recursos[i] != NULL; i++)
    {
        if (!strcmp(name, cfg_kernel->recursos[i]))
            return true;
    }
    return false;
}

void print_resources(t_log *logger)
{
    char *total = string_new();
    pthread_mutex_lock(&MUTEX_LISTA_BLOCKEADOS);
    bool closure(void *void_queue)
    {
        t_blocked_queue *q = (t_blocked_queue *)void_queue;
        return is_resource(q->resource_name);
    }
    t_list *resources = list_filter(_blocked_queues, closure);
    void iterator(void *void_queue)
    {
        t_blocked_queue *queue = (t_blocked_queue *)void_queue;
        string_append_with_format(&total, "Recurso: %s -> %d  -  ", queue->resource_name, queue->instances);
    }
    list_iterate(resources, iterator);
    list_destroy(resources);
    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
    log_info(logger, "%s", total);
    free(total);
}

t_pcb *find_pcb_by_pid(t_sync_queue *queue, uint32_t pid)
{
    bool closure(void *elem)
    {
        t_pcb *pcb = (t_pcb *)elem;
        return pcb->context->pid == pid;
    }
    return sync_queue_find_elem(queue, closure);
}

t_pcb *remove_pcb_by_pid(t_sync_queue *queue, uint32_t pid)
{
    bool closure(void *elem)
    {
        t_pcb *pcb = (t_pcb *)elem;
        return pcb->context->pid == pid;
    }
    return sync_queue_remove_by_condition(queue, closure);
}

t_pcb *remove_pcb_from_blocked_queues_by_pid(uint32_t pid, t_log *logger)
{
    // PERDON DIOS POR LO QUE VAS A VER EN ESTA FUNCION
    t_list_iterator *queues_list_iterator = list_iterator_create(_blocked_queues);
    pthread_mutex_lock(&MUTEX_LISTA_BLOCKEADOS);
    while (list_iterator_has_next(queues_list_iterator))
    {
        t_blocked_queue *block_queue = list_iterator_next(queues_list_iterator);
        t_list *pcb_list = block_queue->block_queue->queue->elements;
        t_list_iterator *pcbs_iterator = list_iterator_create(pcb_list);

        // target = find_pcb_by_pid(block_queue->block_queue, pid);
        pthread_mutex_lock(&block_queue->block_queue->mutex);
        while (list_iterator_has_next(pcbs_iterator))
        {
            t_pcb *target = list_iterator_next(pcbs_iterator);
            if (target->context->pid == pid)
            {
                if (is_resource(block_queue->resource_name))
                {
                    pthread_mutex_lock(&block_queue->resource_mutex);
                    block_queue->instances++;
                    pthread_mutex_unlock(&block_queue->resource_mutex);
                    list_remove_element(pcb_list, target);
                    sem_wait(&block_queue->sem_process_count);
                    pthread_mutex_unlock(&block_queue->block_queue->mutex);
                    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
                    list_iterator_destroy(pcbs_iterator);
                    list_iterator_destroy(queues_list_iterator);
                    return target;
                }
                if (list_iterator_index(pcbs_iterator) == 0)
                {
                    // caso en el medio IO request
                    target->sigterm = true;
                    pthread_mutex_unlock(&block_queue->block_queue->mutex);
                    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
                    list_iterator_destroy(pcbs_iterator);
                    list_iterator_destroy(queues_list_iterator);
                    return target;
                }
                list_remove_element(pcb_list, target);
                sem_wait(&block_queue->sem_process_count);
                t_interface *interface = interface_get(block_queue->resource_name);
                t_packet *p = sync_queue_remove(interface->msg_queue, list_iterator_index(pcbs_iterator));
                packet_free(p);
                pthread_mutex_unlock(&block_queue->block_queue->mutex);
                pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
                list_iterator_destroy(pcbs_iterator);
                list_iterator_destroy(queues_list_iterator);
                return target;
            }
        }
        list_iterator_destroy(pcbs_iterator);
        pthread_mutex_unlock(&block_queue->block_queue->mutex);
    }
    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
    list_iterator_destroy(queues_list_iterator);
    return NULL;
}

void remove_and_destroy_blocked_queue(t_blocked_queue *queue)
{
    pthread_mutex_lock(&MUTEX_LISTA_BLOCKEADOS);
    list_remove_element(_blocked_queues, queue);
    pthread_mutex_unlock(&MUTEX_LISTA_BLOCKEADOS);
    blocked_queue_destroy(queue);
}