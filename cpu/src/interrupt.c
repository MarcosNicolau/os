#include <interrupt.h>

static int int_flag = 0;

static pthread_mutex_t interrupt_mutex;

uint8_t interrupt_reason;

void set_interrupt(void);

void process_interrupt_conn(int fd, t_log *logger)
{
    while (1)
    {
        t_packet *packet = packet_new(0);
        if (packet_recv(fd, packet) == -1)
        {
            packet_free(packet);
            log_error(logger, "Se desconecto el kernel de interrupt\n"); // cambiar por log
            break;
        }
        interrupt_reason = packet->op_code;
        set_interrupt();
        // log_debug(logger,"hola estoy aca");
        packet_free(packet);
    }
}

void handle_interrupt(void *_args)
{
    t_process_conn_args *args = _args;
    t_log *logger = args->logger;
    int server_fd = args->fd;

    free(args);

    while (1)
    {
        int client_fd = socket_acceptConns(server_fd);
        if (client_fd == -1)
            continue;

        process_interrupt_conn(client_fd, logger);
    }
}

void interrupt_mutex_init(void)
{
    pthread_mutex_init(&interrupt_mutex, NULL);
}

void interrupt_mutex_destroy(void)
{
    pthread_mutex_destroy(&interrupt_mutex);
}

void set_interrupt(void)
{
    pthread_mutex_lock(&interrupt_mutex);
    int_flag = 1;
    pthread_mutex_unlock(&interrupt_mutex);
}

void clear_interrupt(void)
{
    pthread_mutex_lock(&interrupt_mutex);
    int_flag = 0;
    pthread_mutex_unlock(&interrupt_mutex);
}

int interrupt_flag(void)
{
    pthread_mutex_lock(&interrupt_mutex);
    int value = int_flag;
    pthread_mutex_unlock(&interrupt_mutex);
    return value;
}