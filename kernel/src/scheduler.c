#include <scheduler.h>

static void set_scheduling_algorithm(void);

// para pausar la planificacion
static int paused_threads = 0;
static pthread_mutex_t MUTEX_PAUSE;
static bool scheduler_paused = false;
static sem_t sem_paused;

// para el grado de multiprogramacion
static sem_t current_multiprogramming_grade;
static int current_multiprogramming_sem_mirror;
static pthread_mutex_t current_multiprogramming_grade_mutex;

static uint32_t max_multiprogramming_grade;
static pthread_mutex_t max_multiprogramming_grade_mutex;

static uint32_t processes_in_memory_amount = 0;
static pthread_mutex_t processes_in_memory_amount_mutex;

int sigterm_new = 0;

t_scheduler scheduler;

static void init_scheduler_sems(void)
{
    sem_init(&scheduler.sem_ready, 0, 0);
    sem_init(&scheduler.sem_new, 0, 0);

    sem_init(&sem_paused, 0, 0);
    pthread_mutex_init(&MUTEX_PAUSE, NULL);

    pthread_mutex_init(&processes_in_memory_amount_mutex, NULL);
    pthread_mutex_init(&max_multiprogramming_grade_mutex, NULL);
    pthread_mutex_init(&current_multiprogramming_grade_mutex, NULL);
}

static void destroy_scheduler_sems(void)
{
    sem_destroy(&scheduler.sem_ready);
    sem_destroy(&scheduler.sem_new);
    sem_destroy(&sem_paused);
    sem_destroy(&current_multiprogramming_grade);
    pthread_mutex_destroy(&MUTEX_PAUSE);
    pthread_mutex_destroy(&max_multiprogramming_grade_mutex);
    pthread_mutex_destroy(&processes_in_memory_amount_mutex);
    pthread_mutex_destroy(&current_multiprogramming_grade_mutex);
}

void handle_pause(void)
{
    bool res;
    pthread_mutex_lock(&MUTEX_PAUSE);
    if ((res = scheduler_paused))
    {
        paused_threads++;
    }
    pthread_mutex_unlock(&MUTEX_PAUSE);
    if (res)
        sem_wait(&sem_paused);
}

void pause_threads(void)
{
    pthread_mutex_lock(&MUTEX_PAUSE);
    scheduler_paused = true;
    pthread_mutex_unlock(&MUTEX_PAUSE);
}

void resume_threads(void)
{
    pthread_mutex_lock(&MUTEX_PAUSE);
    if (scheduler_paused)
    {
        scheduler_paused = false;
        while (paused_threads > 0)
        {
            sem_post(&sem_paused);
            paused_threads--;
        }
    }
    pthread_mutex_unlock(&MUTEX_PAUSE);
}

void init_scheduler(void)
{
    max_multiprogramming_grade = cfg_kernel->grado_multiprogramacion;
    init_scheduler_sems();
    set_scheduling_algorithm();
}

void destroy_scheduler(void)
{
    destroy_scheduler_sems();
}

static void set_scheduling_algorithm(void)
{
    if (!strcmp(cfg_kernel->algoritmo_planificacion, "FIFO"))
    {
        scheduler.ready_to_exec = ready_to_exec_fifo;
        scheduler.dispatch = dispatch_fifo;
        scheduler.block_to_ready = block_to_ready_fifo;
        scheduler.move_pcb_to_blocked = move_pcb_to_blocked_fifo;
        return;
    }
    if (!strcmp(cfg_kernel->algoritmo_planificacion, "RR"))
    {
        scheduler.ready_to_exec = ready_to_exec_rr;
        scheduler.dispatch = dispatch_rr;
        scheduler.block_to_ready = block_to_ready_rr;
        scheduler.exec_to_ready = exec_to_ready_rr;
        scheduler.move_pcb_to_blocked = move_pcb_to_blocked_rr;
        return;
    }
    scheduler.ready_to_exec = ready_to_exec_vrr;
    scheduler.dispatch = dispatch_vrr;
    scheduler.exec_to_ready = exec_to_ready_vrr;
    scheduler.block_to_ready = block_to_ready_vrr;
    scheduler.move_pcb_to_blocked = move_pcb_to_blocked_vrr;
}

void handle_short_term_scheduler(void *args_logger)
{
    t_log *logger = (t_log *)args_logger;
    set_scheduling_algorithm();
    log_info(logger, "El algoritmo seleccionado fue: %s", cfg_kernel->algoritmo_planificacion);

    while (1)
    {
        handle_pause();
        sem_wait(&scheduler.sem_ready);
        t_pcb *pcb = scheduler.ready_to_exec();
        log_info(logger, "PID: %d - Estado Anterior: READY - Estado Actual: EXEC", pcb->context->pid); // solo lo saco, la referencia creo que ya la tengo
        scheduler.dispatch(pcb, logger);
        queue_sync_pop(exec_queue);
    }
}

static void inc_processes_in_memory_amount(void)
{
    pthread_mutex_lock(&processes_in_memory_amount_mutex);
    processes_in_memory_amount++;
    pthread_mutex_unlock(&processes_in_memory_amount_mutex);
}

static void dec_processes_in_memory_amount(void)
{
    pthread_mutex_lock(&processes_in_memory_amount_mutex);
    processes_in_memory_amount--;
    pthread_mutex_unlock(&processes_in_memory_amount_mutex);
}

void change_multiprogramming(int new_grade)
{
    pthread_mutex_lock(&current_multiprogramming_grade_mutex);
    if (new_grade >= max_multiprogramming_grade)
    {
        for (int i = 0; i < new_grade - max_multiprogramming_grade; i++)
        {
            current_multiprogramming_sem_mirror++;
            sem_post(&current_multiprogramming_grade);
        }
    }
    else
    {
        pthread_mutex_lock(&processes_in_memory_amount_mutex);
        int target_sem_value;
        if ((target_sem_value = (new_grade - processes_in_memory_amount)) <= 0)
            target_sem_value = 0;
        while (current_multiprogramming_sem_mirror > target_sem_value)
        {
            sem_wait(&current_multiprogramming_grade);
            current_multiprogramming_sem_mirror--;
        }
        pthread_mutex_unlock(&processes_in_memory_amount_mutex);
    }
    pthread_mutex_unlock(&current_multiprogramming_grade_mutex);

    pthread_mutex_lock(&max_multiprogramming_grade_mutex);
    max_multiprogramming_grade = new_grade;
    pthread_mutex_unlock(&max_multiprogramming_grade_mutex);
}

void instr_signal(t_pcb *pcb, t_blocked_queue *queue, t_log *logger)
{
    int *taken = dictionary_get(pcb->taken_resources, queue->resource_name);
    *taken = (*taken - 1) > 0 ? (*taken - 1) : 0;
    pthread_mutex_lock(&queue->resource_mutex);
    queue->instances++;
    pthread_mutex_unlock(&queue->resource_mutex);
    if (queue->instances <= 0 && sync_queue_length(queue->block_queue) > 0)
    {
        scheduler.block_to_ready(queue, logger);
    }
}

void move_pcb_to_exit(t_pcb *pcb, t_log *logger)
{

    queue_sync_push(exit_queue, pcb);
    send_end_process(pcb->context->pid);
    recv_end_process(logger);
    log_info(logger, "PID: %d - Estado Anterior: %s - Estado Actual: EXIT", pcb->context->pid, pcb_state_to_string(pcb));
    pcb->state = EXIT;
    pthread_mutex_lock(&max_multiprogramming_grade_mutex);
    pthread_mutex_lock(&processes_in_memory_amount_mutex);
    if (processes_in_memory_amount <= max_multiprogramming_grade)
    {
        pthread_mutex_lock(&current_multiprogramming_grade_mutex);
        current_multiprogramming_sem_mirror++;
        sem_post(&current_multiprogramming_grade);
        pthread_mutex_unlock(&current_multiprogramming_grade_mutex);
    }
    pthread_mutex_unlock(&processes_in_memory_amount_mutex);
    dec_processes_in_memory_amount();
    pthread_mutex_unlock(&max_multiprogramming_grade_mutex);

    // le libero todos los recursos

    void iterator(char *name,void* elem){
        int *taken = (int*) elem;
        t_blocked_queue *q = get_blocked_queue_by_name(name);
        while (*taken > 0)
        {
            instr_signal(pcb, q, logger);
        }
    }
   dictionary_iterator(pcb->taken_resources,iterator);
}


void block_to_exit(t_blocked_queue* queue, t_log *logger)
{
    //wait sem cola bloqueado
    t_pcb *pcb = blocked_queue_pop(queue);
    if(!pcb)
        log_error(logger,"blocked queue not found");
    log_info(logger, "Finaliza el proceso %d - Motivo: Error de interfaz DIALFS", pcb->context->pid);
    move_pcb_to_exit(pcb, logger);
}

void handle_long_term_scheduler(void *args_logger)
{
    sem_init(&current_multiprogramming_grade, 0, max_multiprogramming_grade);
    current_multiprogramming_sem_mirror = max_multiprogramming_grade;
    t_log *logger = (t_log *)args_logger;

    while (1)
    {
        // todo esto esto es para hacer el  wait llevando el valor del semaforo
        // ----

        pthread_mutex_lock(&current_multiprogramming_grade_mutex);
        current_multiprogramming_sem_mirror--;
        pthread_mutex_unlock(&current_multiprogramming_grade_mutex);
        sem_wait(&current_multiprogramming_grade);

        sem_wait(&scheduler.sem_new);
        handle_pause();
        while (sigterm_new > 0)
        {
            sem_wait(&scheduler.sem_new);
            handle_pause();
            sigterm_new--;
        }

        t_pcb *pcb = queue_sync_pop(new_queue);

        send_create_process(pcb);
        if (recv_create_process(logger))
        {
            log_info(logger, "Finaliza el proceso %d- Motivo: No se encontro archivo de pseudocodigo", pcb->context->pid);
            pthread_mutex_lock(&current_multiprogramming_grade_mutex);
            current_multiprogramming_sem_mirror++;
            pthread_mutex_unlock(&current_multiprogramming_grade_mutex);
            sem_post(&current_multiprogramming_grade);
            queue_sync_push(exit_queue,pcb);
            log_info(logger, "PID: %d - Estado Anterior: NEW - Estado Actual: EXIT", pcb->context->pid);
            continue;
        }
        pcb->state = READY;
        queue_sync_push(ready_queue, pcb);
        log_info(logger, "PID: %d - Estado Anterior: NEW - Estado Actual: READY", pcb->context->pid);
        print_ready_queue(logger, false);
        sem_post(&scheduler.sem_ready);
        inc_processes_in_memory_amount();
    }
}

bool handle_sigterm(t_pcb *pcb, t_log *logger)
{
    if (pcb->sigterm)
    {
        log_info(logger, "Finaliza el proceso %d- Motivo: INTERRUPTED_BY_USER", pcb->context->pid);
        move_pcb_to_exit(pcb, logger);
        return true;
    }
    return false;
}