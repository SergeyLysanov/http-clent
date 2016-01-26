#ifndef URL_H
#define URL_H

enum url_scheme {
    SCHEME_HTTP = 0,
    SCHEME_HTTPS, /* Unsuported yet */
    SCHEME_INVALID
};

/* scheme:[//[user:password@]host[:port]][/]path[?query][#fragment] */
typedef struct url {
    enum url_scheme scheme;
    char *user;
    char *password;
    char *host;
    char *port;
    char *path;
    char *query;
    char *fragment;
    char *file;
} url_t;

url_t* parse_url(const char *url, int *error);
void print_url(url_t *url);
void print_url_error(int error);
void free_url(url_t *url);

#endif // URL_H
