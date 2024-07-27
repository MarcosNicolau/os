#include <process.h>

static t_list *process_list;

static pthread_mutex_t MUTEX_PROCESS_LIST;

static void clear_all_frames(t_process_in_mem* process);

void init_process_list(void)
{
    process_list = list_create();
    pthread_mutex_init(&MUTEX_PROCESS_LIST, NULL);
}

t_process_in_mem *t_process_in_mem_create(void)
{
    t_process_in_mem *process = malloc(sizeof(t_process_in_mem));
    process->page_table = list_create();
    process->current_size = 0;
    return process;
}

void t_process_in_mem_destroy(t_process_in_mem *process)
{
    // si voy a destruir un proceso primero libero todos sus marcos
    clear_all_frames(process);
    free(process->path);
    list_destroy_and_destroy_elements(process->page_table, free);
    free(process);
}

static void destroyer(void *process)
{
    t_process_in_mem *p = (t_process_in_mem *)process;
    t_process_in_mem_destroy(p);
}

void process_list_destroy(void)
{
    list_destroy_and_destroy_elements(process_list, destroyer);
    pthread_mutex_destroy(&MUTEX_PROCESS_LIST);
}

void packet_get_process_in_mem(t_buffer *buffer, t_process_in_mem *process)
{
    process->pid = packet_getUInt32(buffer);
    char *path = packet_getString(buffer);
    process->path = mount_instructions_directory(path);
    // process->page_table = packet_get_list(buffer);
    free(path);
}

void add_process(t_process_in_mem *process)
{
    pthread_mutex_lock(&MUTEX_PROCESS_LIST);
    list_add(process_list, process);
    pthread_mutex_unlock(&MUTEX_PROCESS_LIST);
}

t_process_in_mem *find_process_by_pid(uint32_t pid)
{
    bool closure(void *elem)
    {
        t_process_in_mem *aux_process = (t_process_in_mem *)elem;
        return aux_process->pid == pid;
    }
    pthread_mutex_lock(&MUTEX_PROCESS_LIST);
    t_process_in_mem *res = list_find(process_list, closure);
    pthread_mutex_unlock(&MUTEX_PROCESS_LIST);
    return res;
}

void remove_process_by_pid(uint32_t pid,t_log* logger)
{
    bool closure(void *elem)
    {
        t_process_in_mem *aux_process = (t_process_in_mem *)elem;
        return aux_process->pid == pid;
    }
    pthread_mutex_lock(&MUTEX_PROCESS_LIST);
    t_process_in_mem* process =  list_remove_by_condition(process_list, closure);
    pthread_mutex_unlock(&MUTEX_PROCESS_LIST);
    if(!process)
        return;
    log_info(logger, "PID: %u - Tamaño: %d", pid, list_size(process->page_table));
    t_process_in_mem_destroy(process);
}

void process_remove_last_page_from_table(t_process_in_mem *process)
{
    uint32_t *frame = list_remove(process->page_table, list_size(process->page_table) - 1);
    clear_frame(*frame); // lo marco como libre
    free(frame);
}

static void clear_all_frames(t_process_in_mem* process){
    void iterator(void* elem){
        uint32_t* frame = (uint32_t*) elem;
        clear_frame(*frame);
    }
    list_iterate(process->page_table,iterator);
}

uint32_t get_frame(t_process_in_mem* process, uint32_t page_number){
    uint32_t *frame =  list_get(process->page_table,page_number);
    return *frame;
}