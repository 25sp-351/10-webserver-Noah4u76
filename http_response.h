#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stddef.h>

// HTTP response status codes
#define HTTP_STATUS_OK               200
#define HTTP_STATUS_BAD_REQUEST      400
#define HTTP_STATUS_NOT_FOUND        404
#define HTTP_STATUS_METHOD_NOT_ALLOWED 405
#define HTTP_STATUS_INTERNAL_ERROR   500

// HTTP response structure
typedef struct {
    int status_code;
    char content_type[128];
    size_t content_length;
    void *body;
} http_response_t;

// Initialize a response structure
void init_http_response(http_response_t *response);

// Set response status
void set_response_status(http_response_t *response, int status_code);

// Set response content type
void set_response_content_type(http_response_t *response, const char *content_type);

// Set response body
int set_response_body(http_response_t *response, const void *body, size_t body_length);

// Set response body from a string
int set_response_body_string(http_response_t *response, const char *body);

// Write response to a buffer
size_t write_http_response(const http_response_t *response, char *buffer, size_t buffer_size);

// Free any allocated memory in the response
void free_http_response(http_response_t *response);

// Get content type based on file extension
const char *get_content_type_for_file(const char *filename);

#endif