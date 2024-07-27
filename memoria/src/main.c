#include <main.h>

static char * MEM_PROPERTIES[] = {"PUERTO_ESCUCHA","TAM_MEMORIA","TAM_PAGINA","PATH_INSTRUCCIONES","RETARDO_RESPUESTA",NULL};
static t_log *logger;
static t_config *config;
t_mem_config *cfg_mem;
static int fd_server;

static void config_init(char* path)
{
    char* mounted_path = mount_config_directory(PROCESS_NAME,path);
    config = config_create(mounted_path);
    free(mounted_path);
    if (!config)
    {
        log_error(logger, "error al cargar el config");
        exit(1);
    }
    if(!config_has_all_properties(config,MEM_PROPERTIES)){
        log_error(logger, "Finalizo la memoria, le faltan atributos al config");
        exit(1);
    }

    cfg_mem = malloc(sizeof(t_mem_config));
    cfg_mem->puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    cfg_mem->tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    cfg_mem->tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    cfg_mem->path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    cfg_mem->retardo_respuesta = config_get_int_value(config, "RETARDO_RESPUESTA");
}

static void init_memory(int argc, char** argv)
{
    logger = log_create(LOG_PATH, PROCESS_NAME, 1, LOG_LEVEL);
    if (!logger)
    {
        perror("error al crear el logger");
        exit(1);
    }

    if(argc < 2){
        log_error(logger,"esperaba %s [CONFIG_PATH]",argv[0]);
        exit(1);
    }

    config_init(argv[1]);

    if(init_ram(cfg_mem->tam_memoria)) {
        log_error(logger,"ERROR MAGISTRAL NO HAY MEMORIA PARA INICAR EL PROGRAMA");
        exit(1);
    }
    init_mem_bitarray(cfg_mem->tam_memoria,cfg_mem->tam_pagina);
    fd_server = socket_createTcpServer(NULL, cfg_mem->puerto_escucha);

    if (fd_server == -1)
    {
        log_error(logger, "error %s", strerror(errno));
        exit(1);
    }

    init_process_list();

    log_info(logger, "server starting");
}

static void memory_close(void)
{
    log_destroy(logger);
    free(cfg_mem);
    config_destroy(config);
    process_list_destroy();
}

void sighandler(int signal)
{
    memory_close();
    close(fd_server);
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sighandler);
    init_memory(argc,argv);
    socket_acceptOnDemand(fd_server, logger, process_conn);
    memory_close();
    return 0;
}
