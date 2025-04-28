#ifndef ROUTE_HANDLER_H
#define ROUTE_HANDLER_H

#include "http_request.h"
#include "http_response.h"

// Handle the incoming request and generate a response
void handle_request(const http_request_t *request, http_response_t *response);

// Handle static file requests
void handle_static_file(const char *path, http_response_t *response);

// Handle calculator requests
void handle_calc(const char *path, http_response_t *response);

// Handle sleep requests for pipeline testing
void handle_sleep(const char *path, http_response_t *response);

#endif