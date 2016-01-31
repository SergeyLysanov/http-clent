#include "url.h"
#include "log.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#define MAX_URL_LENGTH 2048
#define DEFAULT_HTTP_PORT "80"
#define DEFAULT_HTTPS_PORT "423"

struct scheme_data
{
    const char *name;
    const char *default_port;
};

static struct scheme_data supported_schemes[] =
{
    { "http://",  DEFAULT_HTTP_PORT },
    /*{ "https://", DEFAULT_HTTPS_PORT },   Unsupported */
    { NULL, NULL }
};

static const char *url_errors[] = {
#define URL_NO_ERROR                0
  "No error",
#define URL_UNSOPORTED_SCHEME       1
  "Unsupported scheme",
#define URL_INVALID_HOST            2
  "Invalid host name",
#define URL_INVALID_PORT_NUMBER     3
  "Invalid port number",
#define URL_INVALID_USER_NAME       4
  "Invalid user name",
#define URL_INVALID_PATH            5
  "Invalid path",
#define URL_TOO_LONG                6
  "Url too long",
#define URL_BAD_ALLOC               7
  "Bad alloc"
};

int parse_scheme(const char *url, enum url_scheme *scheme)
{
    for (int i = 0; supported_schemes[i].name; ++i)
    {
        if (0 == strncmp (url, supported_schemes[i].name, strlen(supported_schemes[i].name) )) {
            *scheme = (url_scheme) i;
            return URL_NO_ERROR;
        }
    }

    return URL_UNSOPORTED_SCHEME;
}

int parse_user_info(const char *begin, const char *end, char **user, char **password)
{
    const char *separator = strchr(begin, ':');
    if (separator == NULL || separator > end) {
        return URL_INVALID_USER_NAME;
    }

    size_t user_len = separator - begin;
    size_t password_len =  end - separator;

    *user = strndup(begin, user_len);
    *password = strndup(separator+1, password_len);

    if (*user == NULL || *password == NULL)
        return URL_BAD_ALLOC;
    LOG_I("We don't support htpp authentification");
    LOG_D("user=%s password=%s", *user, *password);

    return URL_NO_ERROR;
}

int parse_port(const char *begin, const char *end, char **port)
{
    size_t port_len = end - begin;
    if (port_len > 5) {
        return URL_INVALID_PORT_NUMBER;
    }

    *port = strndup(begin+1, port_len);
    if (*port == NULL) {
        return URL_BAD_ALLOC;
    }

    LOG_D("port=%s", *port);

    return URL_NO_ERROR;
}

int parse_host(const char *begin, const char *end, char **host)
{
    if(end) {
        size_t host_len = end - begin;
        *host = strndup(begin, host_len);
    }
    else {
        *host = strdup(begin);
    }

    if(*host == NULL)
        return URL_BAD_ALLOC;

    LOG_D("host=%s", *host);

    return URL_NO_ERROR;
}

int parse_path(const char *begin, const char *end, char **path, char **file)
{
    char *file_start = 0;

    /* Path is empty. In this case create index.html */
    if(!begin) {
        *path = strdup("/index.html");
        *file = strdup("index.html");
        goto exit;
    }

    if (end) {
        /* In case of query */
        size_t path_len = end - begin;
        *path = strndup(begin, path_len);
    }
    else {
        *path = strdup(begin);
    }

    /* Find file name  in path */
    file_start = strrchr(*path, '/') + 1;
    if (file_start)
        *file = strdup(file_start);

exit:
    if (*file == NULL || *path == NULL)
        return URL_BAD_ALLOC;

    LOG_D("path=%s file=%s", *path, *file);

    return URL_NO_ERROR;
}


url_t* parse_url(const char *url, int *error_)
{
    url_t *u = 0;
    enum url_scheme scheme;
    int error_code;

    const char *user_begin, *user_end;
    const char *host_begin, *host_end;
    const char *port_begin, *port_end;
    const char *path_begin, *path_end;

    u = (url_t *)calloc(1, sizeof(url_t));
    if (!u) {
        error_code = URL_BAD_ALLOC;
        goto err;
    }

    if (strlen(url) > MAX_URL_LENGTH) {
        error_code = URL_TOO_LONG;
        goto err;
    }


    error_code = parse_scheme(url, &scheme);
    if(error_code) { goto err; }
    u->scheme = scheme;

    /* find username and password */
    user_begin = url + strlen(supported_schemes[scheme].name);
    user_end = strchr (user_begin, '@');
    if (!user_end || *user_end != '@') {
        /*user information is empty*/
        user_end = user_begin;
        host_begin = user_begin;
    }
    else {
        error_code = parse_user_info(user_begin, user_end-1, &u->user, &u->password);
        if(error_code) { goto err; }

        host_begin = user_end + 1;
    }

    /* Find path */
    path_begin = strchr(host_begin, '/');
    path_end = strchr(host_begin, '?');
    error_code = parse_path(path_begin, path_end, &u->path, &u->file);
    if(error_code) { goto err; }


    /* Find port */
    port_begin = strchr(host_begin, ':');
    port_end = path_begin;
    /* there is non default port */
    if (port_begin) {
        error_code = parse_port(port_begin, port_end-1, &u->port);
        if(error_code) { goto err; }

        host_end = port_begin;
    }
    else {
        u->port = strdup(supported_schemes[scheme].default_port);
        host_end = path_begin;
    }

    error_code = parse_host(host_begin, host_end, &u->host);
    if(error_code) { goto err; }

    return u;

err:
    if(error_)
        *error_ = error_code;

    free_url(u);

    return NULL;
}

void free_url(url_t *url)
{
    if (!url)
        return;

    if (url->user)
        free(url->user);

    if (url->password)
        free(url->password);

    if (url->host)
        free(url->host);

    if (url->port)
        free(url->port);

    if (url->path)
        free(url->path);

    if (url->query)
        free(url->query);

    if (url->fragment)
        free(url->fragment);

    if (url->file)
        free(url->file);

    free(url);
}

void print_url(url_t *url)
{
    if (!url)
        return;

    LOG_I("scheme: %s", supported_schemes[url->scheme].name);
    LOG_I("user: %s", url->user);
    LOG_I("password: %s", url->password);
    LOG_I("host: %s", url->host);
    LOG_I("port: %s", url->port);
    LOG_I("path: %s", url->path);
    LOG_I("file: %s", url->file);
}

void print_url_error(int error)
{
    if((size_t)error > sizeof(url_errors)) {
        LOG_E("Wrong error number");
        return;
    }

    LOG_E("Error in url parsing: %s", url_errors[error]);
}
