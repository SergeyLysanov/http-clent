#include "buffer.h"

#include "utils.h"
#include <cstdlib>
#include <cstdarg>
#include <cstdio>


buffer_t * buffer_alloc(size_t capacity)
{
    buffer_t *buf = (buffer_t *)calloc(1, sizeof(buffer_t));
    char *content = (char *)calloc(1, capacity*sizeof(char));

    GOTO_ERR_IF(buf == NULL || content == NULL);

    buf->capacity = capacity;
    buf->content = content;

    return buf;

err:
    if (buf) free(buf);
    if (content) free(content);

    return NULL;
}

void buffer_free(buffer_t *buffer)
{
    if(!buffer) return;

    if(buffer->content) free(buffer->content);

    free(buffer);
}

int buffer_append(buffer_t *buffer, char *append, int length)
{
    GOTO_ERR_IF((buffer->actual_size+length+1) > buffer->capacity);

    strncpy(buffer->content + buffer->actual_size, append, length);

    buffer->actual_size += length;

    return 0;
err:
    return -1;
}

int buffer_appendf(buffer_t *buffer, const char *format, ...)
{
    char *tmp = NULL;
    int bytes_written, status;

    va_list argp;
    va_start(argp, format);

    bytes_written = vasprintf(&tmp, format, argp);
    GOTO_ERR_IF(bytes_written < 0);

    va_end(argp);

    status = buffer_append(buffer, tmp, bytes_written);
    GOTO_ERR_IF(status != 0);

    free(tmp);

    return 0;
err:
    if (tmp != NULL)
        free(tmp);

    return -1;
}

char * buffer_to_string(buffer_t *buffer)
{
    char *string = (char *)calloc(1, buffer->actual_size+1);
    GOTO_ERR_IF(string == 0);

    strncpy(string, buffer->content, buffer->actual_size);

    return string;

err:
    return NULL;
}
