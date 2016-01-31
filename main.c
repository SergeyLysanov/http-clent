#include <cstdlib>

#include "http.h"
#include "log.h"

void print_usage()
{
    LOG_I("\n Usage: ./http  http://username:password@example.com:123/path/data \n");
}

int main(int argc, char *argv[])
{
    if(argc != 2) {
        print_usage();
        return 1;
    }

    int error = 0;
    const char *url = argv[1];

    url_t *u = parse_url(url, &error);
    print_url(u);

    if(error) {
        print_url_error(error);
        exit(1);
    }

    connection_t *conn = init_connection(u->host, u->port, &error);
    if(error) {
        print_connection_error(error);
        exit(1);
    }
    print_connection_info(conn);

    http_make_request(conn, u);

    free_connection(conn);
    free_url(u);
}
