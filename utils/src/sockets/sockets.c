#include <sockets/sockets.h>
#include <proto/proto.h>

int iniciar_server(char *ip, char *puerto)
{
    int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    // inicializo hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // recibe la address info
    getaddrinfo(ip, puerto, &hints, &servinfo);

    bool conecto = false;

    // Itera por cada addrinfo devuelto
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_servidor == -1) // fallo de crear socket
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1)
        {
            // Si entra aca fallo el bind
            close(socket_servidor);
            continue;
        }
        // Ni bien conecta uno nos vamos del for
        conecto = true;
        break;
    }
    if (!conecto)
    {
        freeaddrinfo(servinfo);
        return 0;
    }

    // Escuchamos las conexiones entrantes

    listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int socket_createTcpServer(char *host, char *port)
{
    int fd;

    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;

    getaddrinfo(host, port, &hints, &server_info);

    struct addrinfo *addr;

    for (addr = server_info; addr != NULL; addr = addr->ai_next)
    {
        fd = socket(addr->ai_family, addr->ai_socktype,
                    addr->ai_protocol);
        if (fd == -1)
            continue;

        const int enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            close(fd);

        if (bind(fd, addr->ai_addr, addr->ai_addrlen) == 0)
            break; /* Success */

        // this means that the we could not bind the socket, so close it and try next one.
        close(fd);
    }

    freeaddrinfo(server_info);

    if (addr == NULL)
        return -1;

    int listening = listen(fd, SOMAXCONN);

    if (listening == -1)
        return -1;

    return fd;
}

int socket_acceptConns(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(struct sockaddr_in);

    int client_fd = accept(server_fd, (void *)&client_addr, &client_addrlen);

    return client_fd;
}

int socket_connectToServer(char *host, char *port)
{
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;

    getaddrinfo(host, port, &hints, &server_info);

    int fd = 0;
    struct addrinfo *addr;

    for (addr = server_info; addr != NULL; addr = addr->ai_next)
    {
        fd = socket(addr->ai_family, addr->ai_socktype,
                    addr->ai_protocol);
        if (fd == -1)
            continue;

        const int enable = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            close(fd);

        if (connect(fd, addr->ai_addr, addr->ai_addrlen) != -1)
            break;

        close(fd);
    }

    freeaddrinfo(server_info);

    // no address succeeded
    if (addr == NULL)
        return -1;

    return fd;
}

void socket_freeConn(int socket_cliente)
{
    close(socket_cliente);
}

/**
 * @param requestHandler pass NULL if you don't expect a response from the server.
 * @returns `-1` when client is closed
 */
int socket_read(int fd, t_requestHandler requestHandler, void *args)
{
    t_packet *packet = packet_new(-1);
    if (packet == NULL)
        return 0;
    // op code and buffer size must always be explicit in the messages
    int bytes_read;
    bytes_read = recv(fd, &(packet->op_code), sizeof(uint8_t), 0);
    bytes_read = recv(fd, &(packet->buffer->size), sizeof(uint32_t), 0);
    packet->buffer->stream = malloc(packet->buffer->size);
    if (packet->buffer->size != 0)
        bytes_read = recv(fd, packet->buffer->stream, packet->buffer->size, 0);

    // no data was sent
    if (bytes_read == -1)
    {
        packet_free(packet);
        return 0;
    }
    // connection closed
    if (bytes_read == 0)
    {
        packet_free(packet);
        return -1;
    }

    // everything alright call the requestHandler
    requestHandler(fd, packet->op_code, packet->buffer, args);
    packet_free(packet);
    return 0;
};

void socket_acceptOnDemand(int fd, t_log *logger, void (*connection_handler)(void *))
{
    while (fd != -1)
    {
        int client_fd = socket_acceptConns(fd);

        if (client_fd == -1)
            continue;

        t_process_conn_args *args = malloc(sizeof(t_process_conn_args));
        if (args == NULL)
        {
            close(client_fd);
            continue;
        }
        args->fd = client_fd;
        args->logger = logger;
        pthread_t client_thread;
        int res = pthread_create(&client_thread, NULL, (void *)connection_handler, (void *)args);
        if (res)
        {
            log_error(logger, "error %s", strerror(errno));
            exit(1);
        }
        pthread_detach(client_thread);
    }
}

int socket_isConnected(int fd)
{
    int error = 0;
    socklen_t len = sizeof(error);
    /**
     * If getsockopt() returns 0 and error is 0,
     * it means the socket is still connected.
     * If error is not 0, it means there was an error, and the socket is not connected anymore.
     */
    int retval = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
    if (retval != 0)
        return 0;

    return (error == 0);
}