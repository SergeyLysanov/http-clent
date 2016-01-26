#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>

typedef struct buffer {
    char* content;
    size_t actual_size;
    size_t capacity;
} buffer_t;

buffer_t * buffer_alloc(size_t capacity);
void buffer_free(buffer_t *buffer);
int buffer_append(buffer_t *buffer, char *append, int length);
int buffer_appendf(buffer_t *buffer, const char *format, ...);
char * buffer_to_string(buffer_t *buffer);

#endif // BUFFER_H

