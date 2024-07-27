#include <main.h>


static char *IO_PROPERTIES[] = {"TIPO_INTERFAZ", "TIEMPO_UNIDAD_TRABAJO", "IP_KERNEL", "PUERTO_KERNEL", "IP_MEMORIA", "PUERTO_MEMORIA", "PATH_BASE_DIALFS", "BLOCK_SIZE", "BLOCK_COUNT", "RETRASO_COMPACTACION", NULL};
static t_log *logger;
static t_config *config;
t_io_config *cfg_io;
char *interface_name;
static char *config_path;
int memory_fd;
int kernel_fd;

static void config_init(void)
{
    char* mounted_path = mount_config_directory(PROCESS_NAME,config_path);
    config = config_create(mounted_path);
    free(mounted_path);
    if (!config)
    {
        log_error(logger,"error al cargar el config");
        exit(1);
    }
    if(!config_has_all_properties(config,IO_PROPERTIES)){
        log_error(logger,"Finalizo el programa, le faltan entradas al config");
        exit(1);
    }
    cfg_io = malloc(sizeof(t_io_config));
    cfg_io->name = interface_name;
    cfg_io->tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");
    cfg_io->unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    cfg_io->ip_kernel = config_get_string_value(config, "IP_KERNEL");
    cfg_io->puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    cfg_io->ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    cfg_io->puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    cfg_io->path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
    cfg_io->block_size = config_get_int_value(config, "BLOCK_SIZE");
    cfg_io->block_count = config_get_int_value(config, "BLOCK_COUNT");
    cfg_io->retraso_compactacion = config_get_int_value(config,"RETRASO_COMPACTACION");
}

static void io_init(int argc, char **argv)
{
    logger = log_create(LOG_PATH, PROCESS_NAME, 1, LOG_LEVEL);
    if (!logger)
    {
        perror("error al crear el logger");
        exit(1);
    }

    // get path and name from arguments
    if (argc < 3)
    {
        log_error(logger, "You must provide the name and the path to the config file");
        exit(1);
    }
    interface_name = argv[1];
    config_path = argv[2];

    config_init();
    if(!strcmp(cfg_io->tipo_interfaz,"DIALFS"))
        load_fs(logger);
}

static void io_close(void)
{
    if(!strcmp(cfg_io->tipo_interfaz,"DIALFS"))
        close_fs();
    socket_freeConn(memory_fd);
    socket_freeConn(kernel_fd);
    log_destroy(logger);
    free(cfg_io);
    config_destroy(config);
}

int main(int argc, char *argv[])
{
    // basic setup
    io_init(argc, argv);

    // connect to servers
    memory_fd = socket_connectToServer(cfg_io->ip_memoria, cfg_io->puerto_memoria);
    kernel_fd = socket_connectToServer(cfg_io->ip_kernel, cfg_io->puerto_kernel);
    if (memory_fd == -1 || kernel_fd == -1)
    {
        log_error(logger, "err: %s", strerror(errno));
        return 1;
    }
    log_info(logger, "connected to server");

    // send registration to kernel
    int res = registerResourceInKernel(kernel_fd, logger, cfg_io);
    if (res == -1)
    {
        log_error(logger, "could not register resource to the kernel, reason: %s", strerror(errno));
        exit(1);
    }

    int status = 0;

    struct kernel_incoming_message_args args;
    args.kernel_fd = kernel_fd;
    args.memory_fd = memory_fd;
    args.logger = logger;
    args.config = cfg_io;

    while (status != -1)
    {
        status = socket_read(kernel_fd, handleKernelIncomingMessage, (void *)&args);
    }

    log_error(logger, "lost connection with the kernel");

    io_close();

    return 0;
}
