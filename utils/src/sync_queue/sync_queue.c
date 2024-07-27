#include <sync_queue/sync_queue.h>

t_sync_queue *sync_queue_create(void)
{
    t_sync_queue *s_queue = malloc(sizeof(t_sync_queue));
    s_queue->queue = queue_create();
    pthread_mutex_init(&s_queue->mutex, NULL);
    return s_queue;
}

void queue_sync_push(t_sync_queue *self, void *element)
{
    pthread_mutex_lock(&self->mutex);
    queue_push(self->queue, element);
    pthread_mutex_unlock(&self->mutex);
}

void *queue_sync_pop(t_sync_queue *self)
{
    pthread_mutex_lock(&self->mutex);
    void *elem = queue_pop(self->queue);
    pthread_mutex_unlock(&self->mutex);
    return elem;
}

void sync_queue_destroy(t_sync_queue *self)
{
    pthread_mutex_destroy(&self->mutex);
    queue_destroy(self->queue);
    free(self);
}

void sync_queue_destroy_with_destroyer(t_sync_queue *self, void (*destroyer)(void *))
{
    pthread_mutex_destroy(&self->mutex);
    queue_destroy_and_destroy_elements(self->queue, destroyer);
    free(self);
}

int sync_queue_length(t_sync_queue *self)
{
    pthread_mutex_lock(&self->mutex);
    int length = queue_size(self->queue);
    pthread_mutex_unlock(&self->mutex);
    return length;
}

void *sync_queue_peek(t_sync_queue *self, int index)
{
    pthread_mutex_lock(&self->mutex);
    void *elem = list_get(self->queue->elements, index);
    pthread_mutex_unlock(&self->mutex);
    return elem;
}

void *sync_queue_find_elem(t_sync_queue *self, bool (*closure)(void *))
{
    pthread_mutex_lock(&self->mutex);
    void *elem = list_find(self->queue->elements, closure);
    pthread_mutex_unlock(&self->mutex);
    return elem;
}

void *sync_queue_remove_by_condition(t_sync_queue *self, bool (*closure)(void *))
{
    pthread_mutex_lock(&self->mutex);
    void *elem = list_remove_by_condition(self->queue->elements, closure);
    pthread_mutex_unlock(&self->mutex);
    return elem;
}

bool sync_queue_remove_element(t_sync_queue* self, void* elem){
    pthread_mutex_lock(&self->mutex);
    bool res = list_remove_element(self->queue->elements,elem);
    pthread_mutex_unlock(&self->mutex);
    return res;
}

void* sync_queue_remove(t_sync_queue* self, int index){
    pthread_mutex_lock(&self->mutex);
    void* res = list_remove(self->queue->elements,index);
    pthread_mutex_unlock(&self->mutex);
    return res;
}

void sync_queue_iterate(t_sync_queue *self, void (*iterator)(void *))
{
    pthread_mutex_lock(&self->mutex);
    list_iterate(self->queue->elements, iterator);
    pthread_mutex_unlock(&self->mutex);
}

void sync_queue_clean(t_sync_queue *self)
{
    pthread_mutex_lock(&self->mutex);
    list_clean(self->queue->elements);
    pthread_mutex_unlock(&self->mutex);
}