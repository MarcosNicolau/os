#include <main.h>

static char * CPU_PROPERTIES[] = {"IP_MEMORIA","PUERTO_MEMORIA","PUERTO_ESCUCHA_DISPATCH","PUERTO_ESCUCHA_INTERRUPT","CANTIDAD_ENTRADAS_TLB","ALGORITMO_TLB",NULL};
t_log *logger;
static int dispatch_fd;
static int interrupt_fd;
int cli_dispatch_fd = -1;
static t_config *config;
static t_cpu_config *cfg_cpu;
static pthread_t thread_intr;
t_exec_context context;


static void config_init(char* path)
{
    char* mounted_path = mount_config_directory(PROCESS_NAME,path);
    config = config_create(mounted_path);
    free(mounted_path);
    if (!config)
    {
        log_error(logger,"error al cargar el config");
        exit(1);
    }
    if(!config_has_all_properties(config,CPU_PROPERTIES)){
        log_error(logger,"Finalizo el cpu, le fatan atributos al config");
        exit(1);
    }
    cfg_cpu = malloc(sizeof(t_cpu_config));

    cfg_cpu->ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    cfg_cpu->puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    cfg_cpu->puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    cfg_cpu->puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    cfg_cpu->cantidad_entradas_tlb = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
    cfg_cpu->algoritmo_tlb = config_get_string_value(config, "ALGORITMO_TLB");

}

static void cpu_init(int argc, char** argv)
{
    if(argc < 2){
        log_error(logger,"esperaba %s [CONFIG_PATH]",argv[0]);
        exit(1);
    }
    logger = log_create(LOG_PATH, PROCESS_NAME, 1, LOG_LEVEL);
    if (!logger)
    {
        perror("error al crear el logger");
        exit(1);
    }
    config_init(argv[1]);

    tlb_init(cfg_cpu->cantidad_entradas_tlb,cfg_cpu->algoritmo_tlb);

    dispatch_fd = socket_createTcpServer(NULL, cfg_cpu->puerto_escucha_dispatch);
    interrupt_fd = socket_createTcpServer(NULL, cfg_cpu->puerto_escucha_interrupt);

    if (dispatch_fd == -1 || interrupt_fd == -1)
    {
        log_error(logger,"error: %s", strerror(errno));
        exit(1);
    }
    log_info(logger, "server starting");
}

static void start_interrupt_listener(void)
{
    interrupt_mutex_init();
    // hago el accept y el manejo de las interrupts en otro hilo

    t_process_conn_args *interrupt_args = malloc(sizeof(t_process_conn_args));
    interrupt_args->fd = interrupt_fd;
    interrupt_args->logger = logger;

    // TODO: agregar la queue en los argumentos y un mutex

    pthread_create(&thread_intr, NULL, (void *)handle_interrupt, (void *)interrupt_args);
    pthread_detach(thread_intr);
}

static void cpu_close(void)
{
    interrupt_mutex_destroy();
    pthread_cancel(thread_intr);
    log_destroy(logger);
    free(cfg_cpu);
    config_destroy(config);
}

void sighandler(int signal)
{
    cpu_close();
    close(dispatch_fd);
    close(interrupt_fd);
    exit(0);
}

int main(int argc, char *argv[])
{

    signal(SIGINT, sighandler);
    // dejo en listen los fds dispatch e interrupt
    cpu_init(argc,argv);
    start_interrupt_listener();

    // me conecto a memoria
    fd_memory = socket_connectToServer(cfg_cpu->ip_memoria, cfg_cpu->puerto_memoria);

    send_mem_handshake(logger);
    // intento recibir la conexion del kernel
    while (cli_dispatch_fd == -1)
    {
        cli_dispatch_fd = socket_acceptConns(dispatch_fd);

        // arranco el ciclo...
        while (1)
        {
            current_exec_process_has_finished = 0;
            if (wait_for_context(&context) == -1)
            {
                log_error(logger, "se desconecto el kernel de dispatch");
                cli_dispatch_fd = -1;
                break;
            }
            log_debug(logger, "me llego: pid: %d, quantum: %d, AX: %u", context.pid, context.quantum, context.registers.ax);
            clear_interrupt();
            while (!current_exec_process_has_finished)
            {
                char *next_instruction = fetch(fd_memory, logger);
                if(!next_instruction)
                    break;
                context.registers.pc++;
                decode_and_execute(next_instruction, logger);
                check_interrupt(logger);
            }
        }
    }

    // cpu_close();

    return 0;
}
