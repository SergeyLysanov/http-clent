#ifndef CONNECT_H
#define CONNECT_H

#include <sys/socket.h>

typedef void (*cb)(void *, const char *, int bytes);

typedef struct connection {
    char *host;
    char *port;
    struct addrinfo *addr_info;
    int sockfd;
    bool opened;
    /* Callback information */
    void *context;
    cb process_response;
} connection_t;

connection_t * init_connection(const char *host, const char *port, int *error);
void open_connection(connection_t *conn, int *error);
void close_connection(connection_t *conn);
void free_connection(connection_t *conn);
int send_all(connection_t *conn, const char *buf, int len, int flags);
int recv_all(connection_t *conn, int flags);
void print_connection_info(const connection_t *conn);
void print_connection_error(int error);

#endif // CONNECT_H

