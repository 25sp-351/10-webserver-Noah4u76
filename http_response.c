#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "http_response.h"

// Status code messages
static const char *get_status_message(int status_code) {
    switch (status_code) {
        case HTTP_STATUS_OK:
            return "OK";
        case HTTP_STATUS_BAD_REQUEST:
            return "Bad Request";
        case HTTP_STATUS_NOT_FOUND:
            return "Not Found";
        case HTTP_STATUS_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case HTTP_STATUS_INTERNAL_ERROR:
            return "Internal Server Error";
        default:
            return "Unknown";
    }
}

void init_http_response(http_response_t *response) {
    if (response) {
        memset(response, 0, sizeof(http_response_t));
        response->status_code = HTTP_STATUS_OK;
        strcpy(response->content_type, "text/plain");
    }
}

void set_response_status(http_response_t *response, int status_code) {
    if (response) {
        response->status_code = status_code;
    }
}

void set_response_content_type(http_response_t *response, const char *content_type) {
    if (response && content_type) {
        strncpy(response->content_type, content_type, sizeof(response->content_type) - 1);
    }
}

int set_response_body(http_response_t *response, const void *body, size_t body_length) {
    if (!response) {
        return -1;
    }
    
    // Free any existing body
    if (response->body) {
        free(response->body);
        response->body = NULL;
        response->content_length = 0;
    }
    
    // Handle empty body
    if (!body || body_length == 0) {
        return 0;
    }
    
    // Allocate and copy new body
    response->body = malloc(body_length);
    if (!response->body) {
        return -1;
    }
    
    memcpy(response->body, body, body_length);
    response->content_length = body_length;
    
    return 0;
}

int set_response_body_string(http_response_t *response, const char *body) {
    if (!response) {
        return -1;
    }
    
    // Handle null or empty string
    if (!body || body[0] == '\0') {
        if (response->body) {
            free(response->body);
            response->body = NULL;
            response->content_length = 0;
        }
        return 0;
    }
    
    return set_response_body(response, body, strlen(body));
}

size_t write_http_response(const http_response_t *response, char *buffer, size_t buffer_size) {
    if (!response || !buffer || buffer_size == 0) {
        return 0;
    }
    
    // Format the status line and headers into the buffer
    int header_len = 0;
    
    // Start with the status line
    header_len += snprintf(buffer + header_len, buffer_size - header_len,
                         "HTTP/1.1 %d %s\r\n", 
                         response->status_code, 
                         get_status_message(response->status_code));
    
    // Add Content-Type header
    header_len += snprintf(buffer + header_len, buffer_size - header_len,
                         "Content-Type: %s\r\n", 
                         response->content_type);
    
    // Add Content-Length header
    header_len += snprintf(buffer + header_len, buffer_size - header_len,
                         "Content-Length: %zu\r\n", 
                         response->content_length);
    
    // Add Connection header
    header_len += snprintf(buffer + header_len, buffer_size - header_len,
                         "Connection: close\r\n");
    
    // End headers with an empty line
    header_len += snprintf(buffer + header_len, buffer_size - header_len, "\r\n");
    
    if (header_len < 0 || (size_t)header_len >= buffer_size) {
        return 0;
    }
    
    // Copy the body if there is one
    if (response->body && response->content_length > 0) {
        size_t remaining_space = buffer_size - header_len;
        if (remaining_space >= response->content_length) {
            memcpy(buffer + header_len, response->body, response->content_length);
            return header_len + response->content_length;
        }
    }
    
    return header_len;
}

void free_http_response(http_response_t *response) {
    if (response && response->body) {
        free(response->body);
        response->body = NULL;
        response->content_length = 0;
    }
}

const char *get_content_type_for_file(const char *filename) {
    if (!filename) {
        return "application/octet-stream";
    }
    
    // Find the file extension
    const char *extension = strrchr(filename, '.');
    if (!extension) {
        return "application/octet-stream";
    }
    
    extension++; // Skip the dot
    
    // Match common file extensions
    if (strcasecmp(extension, "html") == 0 || strcasecmp(extension, "htm") == 0) {
        return "text/html";
    } else if (strcasecmp(extension, "txt") == 0) {
        return "text/plain";
    } else if (strcasecmp(extension, "css") == 0) {
        return "text/css";
    } else if (strcasecmp(extension, "js") == 0) {
        return "application/javascript";
    } else if (strcasecmp(extension, "json") == 0) {
        return "application/json";
    } else if (strcasecmp(extension, "jpg") == 0 || strcasecmp(extension, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(extension, "png") == 0) {
        return "image/png";
    } else if (strcasecmp(extension, "gif") == 0) {
        return "image/gif";
    } else if (strcasecmp(extension, "svg") == 0) {
        return "image/svg+xml";
    } else if (strcasecmp(extension, "pdf") == 0) {
        return "application/pdf";
    } else if (strcasecmp(extension, "xml") == 0) {
        return "application/xml";
    }
    
    return "application/octet-stream";
}