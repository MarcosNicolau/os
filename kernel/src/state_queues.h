#ifndef STATE_QUEUES_H
#define STATE_QUEUES_H

#include <sync_queue/sync_queue.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <semaphore.h>
#include <utils.h>
#include <pcb.h>
#include <config.h>
#include <interface.h>


typedef struct
{
    char *resource_name;
    union
    {
        int fd;
        int instances;
    };
    sem_t sem_process_count;
    t_sync_queue *block_queue;
    pthread_mutex_t resource_mutex;
} t_blocked_queue;

extern t_sync_queue *new_queue;
extern t_sync_queue *ready_queue;
extern t_sync_queue *ready_plus_queue;
extern t_sync_queue *exec_queue;
extern t_sync_queue *exit_queue;
extern pthread_mutex_t MUTEX_LISTA_BLOCKEADOS;

void init_queues(void);
void destroy_queues(void);


t_blocked_queue* add_blocked_queue(char *resource_name, int value);
int blocked_queue_push(char* resource_name, void* elem);
void *blocked_queue_pop(t_blocked_queue* queue);




void blocked_queue_destroy(t_blocked_queue *q);
void remove_and_destroy_blocked_queue(t_blocked_queue* queue);
t_blocked_queue *get_blocked_queue_by_name(char *resource_name);
void blocked_queues_iterate(void (*iterator)(void *));



void print_ready_queue(t_log* logger, bool is_ready_plus);

bool is_resource(char* name);

void print_resources(t_log* logger);

t_pcb *find_pcb_by_pid(t_sync_queue *queue, uint32_t pid);
t_pcb *remove_pcb_by_pid(t_sync_queue *queue, uint32_t pid);
t_pcb *remove_pcb_from_blocked_queues_by_pid(uint32_t pid,t_log* logger);

#endif