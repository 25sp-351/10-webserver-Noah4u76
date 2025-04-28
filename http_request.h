#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stddef.h>

typedef struct {
    char method[16];
    char path[1024];
    char version[16];
    char host[256];
    char content_type[128];
    size_t content_length;
    char *body;
} http_request_t;

// Parse an HTTP request from a buffer
int parse_http_request(const char *buffer, size_t buffer_size, http_request_t *request);

// Free any allocated memory in the request
void free_http_request(http_request_t *request);

#endif