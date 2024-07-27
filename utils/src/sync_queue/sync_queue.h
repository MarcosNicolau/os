#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct
{
    t_queue *queue;
    pthread_mutex_t mutex;
} t_sync_queue;

t_sync_queue *sync_queue_create(void);

void queue_sync_push(t_sync_queue *self, void *element);

void *queue_sync_pop(t_sync_queue *self);

void sync_queue_destroy(t_sync_queue *self);

void sync_queue_destroy_with_destroyer(t_sync_queue *self, void (*destroyer)(void *));

int sync_queue_length(t_sync_queue *self);

void *sync_queue_peek(t_sync_queue *self, int index);

void *sync_queue_find_elem(t_sync_queue *self, bool (*closure)(void *));

void *sync_queue_remove_by_condition(t_sync_queue *self, bool (*closure)(void *));

bool sync_queue_remove_element(t_sync_queue* self, void* elem);

void* sync_queue_remove(t_sync_queue* self, int index);

void sync_queue_iterate(t_sync_queue *self, void (*iterator)(void *));

void sync_queue_clean(t_sync_queue *self);

#endif