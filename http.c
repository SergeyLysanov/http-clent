#include "http.h"

#include "log.h"
#include "utils.h"
#include "buffer.h"

#include <cstdlib>

#define REQUEST_BUFF_SIZE 1024

#define HTTP_PROTOCOL "HTTP/1.0"
#define HTTP_CONTENT_LENGTH "Content-Length:"
#define HTTP_TRANSFER_ENCODING "Transfer-Encoding:"
#define HTTP_BODY_SEPARATOR "\r\n\r\n"

http_request_t * build_request(const char *host, const char *path);
void request_free(http_request_t * request);
void response_cb(void *context, const char* buffer, int bytes);

int http_make_request(connection_t *conn, url_t *url)
{
    if (conn == NULL || url == NULL) {
        return -1;
    }

    int error, bytes = 0;
    http_request_t *request = NULL;

    open_connection(conn, &error);
    GOTO_ERR_IF(error);

    request = build_request(conn->host, url->path);
    GOTO_ERR_IF(request == NULL);
    request->url = url;

    /* Initialize context and callback for response processing */
    conn->context = (void*) request;
    conn->process_response = response_cb;

    bytes = send_all(conn, request->request_buf, strlen(request->request_buf), 0);
    GOTO_ERR_IF(bytes == 0);
    LOG_D("Bytes send: %d", bytes);

    bytes = recv_all(conn, 0);
    LOG_D("Bytes received %d", bytes);

    close_connection(conn);

    LOG_I("\nRequest has been completed");

    return 0;

err:
    request_free(request);
    close_connection(conn);

    return -1;
}

http_request_t * build_request(const char *host, const char *path)
{
    char *request_string = NULL;
    http_request_t *request = NULL;
    buffer_t *buf = NULL;
    int status = 0;

    request = (http_request_t *)calloc(1, sizeof(http_request_t));
    GOTO_ERR_IF(request == NULL);

    buf = buffer_alloc(REQUEST_BUFF_SIZE);
    GOTO_ERR_IF(buf == NULL);

    status |= buffer_appendf(buf, "GET %s %s\r\n", path, HTTP_PROTOCOL); /* Use HTTP/1.1 for chunked */
    status |= buffer_appendf(buf, "Host: %s\r\n", host);
    status |= buffer_appendf(buf, "Connection: close\r\n\r\n");
    if (status) {
        LOG_E("Request is too long");
        goto err;
    }

    request_string = buffer_to_string(buf);
    GOTO_ERR_IF(request_string == NULL);

    request->request_buf = request_string;

    buffer_free(buf);

    return request;

err:
    request_free(request);
    buffer_free(buf);

    return NULL;
}

void request_free(http_request_t * request)
{
    if (request == NULL)
        return;

    if(request->request_buf)
        free(request->request_buf);

    if(request->file)
        fclose(request->file);

    free(request);
}

/* Find value of field and return offset*/
size_t fetch_field_value(const char *buffer, const char *field_name, char **field_value)
{
    size_t offset = 0;
    const char *header_begin, *header_end = 0;

    if ((header_begin = strstr(buffer, field_name)) != NULL) {
        header_end = strchr(header_begin, '\n');
        offset = header_end - buffer;
        size_t field_size = header_end - header_begin - strlen(field_name) - 1;

        *field_value = strndup(header_begin + strlen(field_name), field_size);
    }

    return offset;
}

#define HTTP_VERSION    "/1.1 "
#define RESPONSE_CODE_LENGTH 3
size_t fetch_response_code(const char *buffer, int *response_code)
{
    char *field_value = 0;
    size_t offset = fetch_field_value(buffer, "HTTP", &field_value);

    if (field_value) {
        char *code_string = strndup(field_value+strlen(HTTP_VERSION), RESPONSE_CODE_LENGTH);
        if(code_string) {
            *response_code = atoi(code_string);
            free(code_string);
        }
        free(field_value);
    }

    return offset;
}



size_t fetch_content_length(const char *buffer, int *content_length)
{
    char *field_value = 0;
    size_t offset = fetch_field_value(buffer, HTTP_CONTENT_LENGTH, &field_value);

    if(field_value) {
        *content_length = atoi(field_value);
        free(field_value);
    }

    return offset;
}

size_t fetch_transfer_encoding(const char *buffer, bool *chunked_encoding)
{
    char *field_value = 0;
    size_t offset = fetch_field_value(buffer, HTTP_TRANSFER_ENCODING, &field_value);

    if(field_value && (strcmp(field_value, "chunked") == 0)) {
        *chunked_encoding = true;
        free(field_value);
    }

    return offset;
}

/* Note: http headers should fit into CHUNK_SIZE */
size_t parse_header(http_request_t * request, const char *buffer)
{
    size_t offset = 0, body_offset = 0;

    offset += fetch_response_code(buffer+offset, &request->response_code);
    offset += fetch_content_length(buffer+offset, &request->content_length);
    offset += fetch_transfer_encoding(buffer+offset, &request->chunked);

    const char *body = strstr(buffer+offset, HTTP_BODY_SEPARATOR);
    if(body) {
        request->header_parsed = true;
        body_offset = (body - buffer + strlen(HTTP_BODY_SEPARATOR));
    }

    return body_offset;
}

int save_body_to_file(http_request_t * request, const char *buffer, int bytes)
{
    size_t bytes_written = 0;

    /* Need to create file */
    if (request->file == NULL) {

        if(request->url && request->url)
            request->file = fopen(request->url->file, "wb");

        if (request->file == NULL)
            perror("Failed to open file");
    }

    if (request->file)
        bytes_written = fwrite(buffer , sizeof(char), bytes, request->file);

    return bytes_written;
}

size_t parse_chunk_size(http_request_t * request, const char *begin_chunk)
{
    const char *size_end = strchr(begin_chunk, '\n');

    char *size_hex = strndup(begin_chunk, size_end-begin_chunk-1);
    if(size_hex) {
        request->chunk_size = (size_t)strtol(size_hex, NULL, 16);
        free(size_hex);
    }

    return size_end-begin_chunk+1;
}

int save_body_to_file_by_chunks(http_request_t * , const char *, int )
{
    /* Not implemented */
    return 0;
}

void print_buffer(const char *buffer, int bytes)
{
    for(int i = 0; i < bytes; ++i) {
        fprintf(stdout, "%c", buffer[i]);
    }
}

void print_progress(http_request_t *request)
{
    if (request->content_length) {
        int progress = (float)request->written_bytes/request->content_length * 100;
        fprintf(stdout, "\rProgress: %d %%", progress);
        fflush(stdout);
    }
}

void response_cb(void *context, const char *buffer, int bytes)
{
    http_request_t *request = 0;
    size_t body_offset = 0;

    if(!context)
        return;

    request = (http_request_t *)context;

#ifdef DEBUG
    for(int i = 0; i < bytes; ++i) {
        fprintf(stdout, "%c", buffer[i]);
    }
#endif

    /* parse header until body start */
    if (!request->header_parsed) {
        body_offset = parse_header(request, buffer);
        /* Move buffer pointer to body */
        if (body_offset) {
            buffer += body_offset;
            bytes -= body_offset;

            if (request->response_code != HTTP_OK) {
                LOG_E("Bad response code: %d", request->response_code);
            }
        }
    }

    if (request->header_parsed && (request->response_code == HTTP_OK)) {
        if (request->chunked) {
            save_body_to_file_by_chunks(request, buffer, bytes);
        }
        else {
            request->written_bytes += save_body_to_file(request, buffer, bytes);
            print_progress(request);
        }
    }
    else {
        LOG_I("\nContent: ");
        print_buffer(buffer, bytes);
    }
}
