#include <interface.h>

t_dictionary *interface_dictionary;

static pthread_mutex_t INTERFACE_MUTEX;

static void interface_destroyer(void *_interface);

void interface_init(void)
{
    interface_dictionary = dictionary_create();
    pthread_mutex_init(&INTERFACE_MUTEX,0);
}

void interface_add(t_interface *interface)
{
    pthread_mutex_lock(&INTERFACE_MUTEX);
    dictionary_put(interface_dictionary, interface->name, interface);
    pthread_mutex_unlock(&INTERFACE_MUTEX);
}

t_interface *interface_get(char *name)
{
    t_interface* res;
    pthread_mutex_lock(&INTERFACE_MUTEX);
    res= dictionary_get(interface_dictionary, name);
    pthread_mutex_unlock(&INTERFACE_MUTEX);
    return res;
}

t_interface *interface_get_by_fd(int fd)
{
    bool closure(void *elem)
    {
        t_interface *interface = (t_interface *)elem;
        return interface->fd == fd;
    }
    pthread_mutex_lock(&INTERFACE_MUTEX);
    t_list *list = dictionary_elements(interface_dictionary);
    t_interface *tmp = list_find(list, closure);
    pthread_mutex_unlock(&INTERFACE_MUTEX);
    list_destroy(list);
    return tmp;
}

void destroy_interface_dictionary(void)
{
    void close_clients(char *key, void *elem)
    {
        t_interface *interface = (t_interface *)elem;
        socket_freeConn(interface->fd);
    }
    dictionary_iterator(interface_dictionary, close_clients);
    dictionary_destroy_and_destroy_elements(interface_dictionary, interface_destroyer);
    pthread_mutex_destroy(&INTERFACE_MUTEX);
}

int interface_is_connected(t_interface *interface)
{
    return socket_isConnected(interface->fd);
}

int interface_can_run_instruction(t_interface *interface, uint8_t instruction_to_run)
{
    if(IO_GEN_SLEEP == instruction_to_run)
        return strcmp(interface->type, "GENERICA") == 0;
    if(IO_STDIN_READ == instruction_to_run)
        return strcmp(interface->type, "STDIN") == 0;
    if(IO_STDOUT_WRITE == instruction_to_run)
        return strcmp(interface->type, "STDOUT") == 0;
    if(IO_FS_CREATE == instruction_to_run || IO_FS_DELETE == instruction_to_run ||
    IO_FS_TRUNCATE == instruction_to_run || IO_FS_READ == instruction_to_run
    || IO_FS_WRITE == instruction_to_run)
        return strcmp(interface->type, "DIALFS") == 0;
    return 0; 


    // switch (instruction_to_run)
    // {
    // case IO_GEN_SLEEP:
    //     return strcmp(interface->type, "GENERICA") == 0;
    // case IO_STDIN_READ:
    //     return strcmp(interface->type, "STDIN") == 0;
    // case IO_STDOUT_WRITE:
    //     return strcmp(interface->type, "STDOUT") == 0;
    // // the rest of instructions correspond to the file system
    // default:
    //     return strcmp(interface->type, "DIALFS") == 0;
    // }
}

t_interface *interface_validate(char *name, uint8_t instruction_to_run)
{
    t_interface *interface = interface_get(name);

    // check if exists
    if (interface == NULL)
        return NULL;
    
    // comento por ahora pero por si las dudas dejo
    // if (!interface_is_connected(interface))
    // {
    //     interface_destroy(interface);
    //     socket_freeConn(interface->fd);
    //     return NULL;
    // }

    if (!interface_can_run_instruction(interface, instruction_to_run))
        return NULL;

    return interface;
}

void interface_destroy(t_interface *interface)
{
    pthread_mutex_lock(&INTERFACE_MUTEX);
    dictionary_remove_and_destroy(interface_dictionary, interface->name, interface_destroyer);
    pthread_mutex_unlock(&INTERFACE_MUTEX);
}

void packet_destroyer(void* _packet){
    t_packet* p = _packet;
    packet_free(p);
}

static void interface_destroyer(void *_interface)
{
    t_interface *interface = (t_interface *)_interface;
    sync_queue_destroy_with_destroyer(interface->msg_queue,packet_destroyer);
    free(interface->name);
    free(interface->type);
    free(interface);
}