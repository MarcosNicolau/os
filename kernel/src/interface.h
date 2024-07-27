#ifndef INTERFACE_K_H
#define INTERFACE_K_H

#include <commons/collections/dictionary.h>
#include <proto/interface.h>
#include <sockets/sockets.h>
#include <sync_queue/sync_queue.h>
#include <pcb.h>

enum interface_types
{
    GENERIC,
    STDIN,
    STDOUT,
    DIALFS
};

typedef struct
{
    char *name;
    char *type;
    int fd;
    t_sync_queue *msg_queue; // cola de paquetes
} t_interface;

extern t_dictionary *interface_dictionary;

void interface_init(void);
void interface_add(t_interface *interface);
t_interface *interface_validate(char *name, uint8_t instruction_to_run);
t_interface *interface_get(char *name);
t_interface *interface_get_by_fd(int fd);
int interface_is_connected(t_interface *interface);
int interface_can_run_instruction(t_interface *interface, uint8_t instruction_to_run);
void interface_destroy(t_interface *interface);
char *interface_get_type_name(enum interface_types type);
void destroy_interface_dictionary(void);
// t_interface *interface_middleware(t_buffer *buffer, uint8_t instruction_to_run, t_pcb *pcb, t_log *logger);

#endif