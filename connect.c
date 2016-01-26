#include "connect.h"

#include <cstring>
#include <cstdlib>

#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "log.h"

static const char *conn_errors[] = {
#define CONN_NO_ERROR               0
  "No error",
#define CONN_INVALID_HOST           1
  "Something wrong in host",
#define CONN_INVALID_SOCKET         2
  "Failed to create socket",
#define CONN_CONNECT_ERROR          3
  "Failed to connect",
#define CONN_BAD_ALLOC              4
  "Bad alloc"
};

connection_t* init_connection(const char *host, const char *port, int *error)
{
    int status, error_code = 0;
    connection_t *conn;

    struct addrinfo hints, *res;

    conn = (connection_t *)calloc(1, sizeof(connection_t));
    if (!conn) {
        error_code = CONN_BAD_ALLOC;
        goto err;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
        LOG_E("getaddrinfo error: %s", gai_strerror(status));
        error_code = CONN_INVALID_HOST;
        goto err;
    }

    conn->addr_info = res;
    conn->host = strdup(host);
    conn->port = strdup(port);

    if (!conn->host || !conn->port) {
        error_code = CONN_BAD_ALLOC;
        goto err;
    }

    return conn;

err:
    if(error)
        *error = error_code;

    if(conn)
        free(conn);

    return NULL;
}

void open_connection(connection_t *conn, int *error)
{
    int error_code = 0;
    int sockfd;
    struct addrinfo *res = conn->addr_info;

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd <= 0) {
        error_code = CONN_INVALID_SOCKET;
        goto err;
    }

    if(connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connect failed");
        error_code = CONN_CONNECT_ERROR;
        goto err;
    }

    conn->sockfd = sockfd;
    conn->opened = true;

    return;

err:
    if(error)
        *error = error_code;
    close_connection(conn);
}

void close_connection(connection_t *connection)
{
    if(connection->sockfd && connection->opened) {
        close(connection->sockfd);
        connection->opened = false;
    }
}


void free_connection(connection_t *conn)
{
    if(!conn)
        return;

    if (conn->host)
        free(conn->host);

    if (conn->port)
        free(conn->port);

    if(conn->addr_info)
        freeaddrinfo(conn->addr_info);

    free(conn);
}

void print_connection_info(const connection_t *conn)
{
    if (!conn || !conn->addr_info || !conn->host)
        return;

    LOG_I("\nIP addresses for %s:", conn->host);

    struct addrinfo *p;
    char ipstr[INET6_ADDRSTRLEN];
    for(p = conn->addr_info; p != NULL; p = p->ai_next) {
        void *addr;

        if (p->ai_family == AF_INET) { /* IPv4 */
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { /* IPv6 */
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        LOG_I("%s\n", ipstr);
    }
}

int send_all(connection_t *conn, const char *buf, int len, int flags)
{
    int total = 0;
    int sockfd = 0;
    int n;

    GOTO_ERR_IF(conn == NULL || conn->sockfd == 0);

    sockfd = conn->sockfd;

    while(total < len)
    {
        n = send(sockfd, buf+total, len-total, flags);
        if(n == -1) { break; }
        total += n;
    }

    return (n == -1 ? -1 : total);

err:
    LOG_E("Bad pointer");
    return -1;
}

#define CHUNK_SIZE 2048
/* HTTP headers should fit in chunk size. Otherwise increase CHUNK_SIZE */
int recv_all(connection_t *conn, int flags)
{
    int sockfd = 0;
    size_t bytes_received = 0;
    size_t total_size = 0;
    static char chunk[CHUNK_SIZE];

    GOTO_ERR_IF(conn == NULL || conn->sockfd == 0);

    sockfd = conn->sockfd;

    while(1)
    {
        bytes_received = recv(sockfd, chunk , CHUNK_SIZE , flags);
        total_size += bytes_received;

        if( bytes_received == 0) {
            goto exit;
        }
        else if (bytes_received > 0)
        {
            conn->process_response(conn->context, chunk, bytes_received);
        }
    }

exit:
    return total_size;

err:
    LOG_E("Bad pointer");
    return 0;
}


void print_connection_error(int error)
{
    if((size_t)error > sizeof(conn_errors)) {
        LOG_E("Wrong error number");
        return;
    }

    LOG_E("Connection problem: %s", conn_errors[error]);
}
