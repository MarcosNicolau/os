#include <main.h>

static char * KERNEL_PROPERTIES[] = {"PUERTO_ESCUCHA","IP_MEMORIA","IP_CPU","PUERTO_CPU_DISPATCH","PUERTO_CPU_INTERRUPT","ALGORITMO_PLANIFICACION","QUANTUM","RECURSOS","INSTANCIAS_RECURSOS","GRADO_MULTIPROGRAMACION","PATHBASE_SCRIPTS",NULL};
static t_log *logger;
static t_config *config;
static int server_fd;
t_kernel_config *cfg_kernel;
pthread_t LISTENER_THREAD;
static pthread_t short_term_scheduler_thread;
static pthread_t long_term_scheduler_thread;

static void config_init(char *path)
{
    char *mounted_path = mount_config_directory(PROCESS_NAME,path);
    config = config_create(mounted_path);
    free(mounted_path);
    if (!config)
    {
        log_error(logger,"error al crear el config");
        exit(1);
    }
    if(!config_has_all_properties(config,KERNEL_PROPERTIES)){
        log_error(logger,"Finalizo el kernel, le faltan atributos al config");
        exit(1);
    }

    cfg_kernel = malloc(sizeof(t_kernel_config));

    cfg_kernel->puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    cfg_kernel->ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    cfg_kernel->puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    cfg_kernel->ip_cpu = config_get_string_value(config, "IP_CPU");
    cfg_kernel->puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    cfg_kernel->puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    cfg_kernel->algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    cfg_kernel->quantum = config_get_int_value(config, "QUANTUM");
    cfg_kernel->recursos = config_get_array_value(config, "RECURSOS");
    cfg_kernel->instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
    cfg_kernel->grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    cfg_kernel->pathbase_scripts = config_get_string_value(config, "PATHBASE_SCRIPTS");
}

static void init_kernel(int argc, char **argv)
{
    logger = log_create(LOG_PATH, PROCESS_NAME, 0, LOG_LEVEL);
    if (!logger)
    {
        perror("error al crear el logger");
        exit(1);
    }

    if (argc < 2)
    {
        log_error(logger, "esperaba %s [CONFIG_PATH]", argv[0]);
        exit(1);
    }
    config_init(argv[1]);

    // create interface resources
    interface_init();

    server_fd = socket_createTcpServer(NULL, cfg_kernel->puerto_escucha);
    if (server_fd == -1)
    {
        log_error(logger, "error: %s", strerror(errno));
        exit(1);
    }
    static t_process_conn_args args;
    args.fd = server_fd;
    args.logger = logger;
    init_queues();

    // spawn a thread for the server
    pthread_create(&LISTENER_THREAD, NULL, (void *)handle_connections, (void *)&args);
    pthread_detach(LISTENER_THREAD);

    init_scheduler();

    log_info(logger, "server starting");

    // multiprogramacion_actual = cfg_kernel->grado_multiprogramacion;
}

static void kernel_close(void)
{
   if(short_term_scheduler_thread)
        pthread_cancel(short_term_scheduler_thread);
    if(long_term_scheduler_thread)
        pthread_cancel(long_term_scheduler_thread);
    rl_clear_history();
    destroy_scheduler();
    destroy_queues();
    log_destroy(logger);
    destroy_interface_dictionary();
    string_array_destroy(cfg_kernel->recursos);
    string_array_destroy(cfg_kernel->instancias_recursos);
    free(cfg_kernel);
    config_destroy(config);
    socket_freeConn(fd_dispatch);
    socket_freeConn(fd_interrupt);
    socket_freeConn(fd_memory);
    close(server_fd);
}

void sighandler(int signal)
{
    pthread_cancel(LISTENER_THREAD);
    kernel_close();
    exit(0);
}

static void init_scheduler_threads(void)
{
    pthread_create(&long_term_scheduler_thread, NULL, (void *)handle_long_term_scheduler, (void *)logger);
    pthread_detach(long_term_scheduler_thread);

    pthread_create(&short_term_scheduler_thread, NULL, (void *)handle_short_term_scheduler, (void *)logger);
    pthread_detach(short_term_scheduler_thread);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sighandler);
    init_kernel(argc, argv);

    fd_interrupt = socket_connectToServer(cfg_kernel->ip_cpu, cfg_kernel->puerto_cpu_interrupt);
    fd_dispatch = socket_connectToServer(cfg_kernel->ip_cpu, cfg_kernel->puerto_cpu_dispatch);
    fd_memory = socket_connectToServer(cfg_kernel->ip_memoria, cfg_kernel->puerto_memoria);

    if (fd_interrupt == -1 || fd_dispatch == -1 || fd_memory == -1)
    {
        log_error(logger, "err: %s", strerror(errno));
        kernel_close();
        return 1;
    }

    log_info(logger, "connected to server");
    // inicio hilos planificadores
    init_scheduler_threads();

    // inicio la consola
    start_console(logger);

    kernel_close();

    return 0;
}
