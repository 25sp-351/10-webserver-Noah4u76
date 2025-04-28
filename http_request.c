#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http_request.h"

// Parse the request line
static int parse_request_line(const char *line, size_t line_length, http_request_t *request) {
    // Skip any leading whitespace
    size_t pos = 0;
    while (pos < line_length && isspace(line[pos])) {
        pos++;
    }
    
    // Parse the method
    size_t method_start = pos;
    while (pos < line_length && !isspace(line[pos])) {
        pos++;
    }
    
    if (pos == method_start || pos >= line_length) {
        return -1; // Invalid method
    }
    
    size_t method_length = pos - method_start;
    if (method_length >= sizeof(request->method)) {
        return -1; // Method too long
    }
    
    memcpy(request->method, line + method_start, method_length);
    request->method[method_length] = '\0';
    
    // Skip whitespace between method and path
    while (pos < line_length && isspace(line[pos])) {
        pos++;
    }
    
    // Parse the path
    size_t path_start = pos;
    while (pos < line_length && !isspace(line[pos])) {
        pos++;
    }
    
    if (pos == path_start || pos >= line_length) {
        return -1; // Invalid path
    }
    
    size_t path_length = pos - path_start;
    if (path_length >= sizeof(request->path)) {
        return -1; // Path too long
    }
    
    memcpy(request->path, line + path_start, path_length);
    request->path[path_length] = '\0';
    
    // Skip whitespace between path and version
    while (pos < line_length && isspace(line[pos])) {
        pos++;
    }
    
    // Parse the HTTP version
    size_t version_start = pos;
    while (pos < line_length && !isspace(line[pos])) {
        pos++;
    }
    
    if (pos == version_start) {
        return -1; // Invalid version
    }
    
    size_t version_length = pos - version_start;
    if (version_length >= sizeof(request->version)) {
        return -1; // Version too long
    }
    
    memcpy(request->version, line + version_start, version_length);
    request->version[version_length] = '\0';
    
    return 0;
}

//atoi implementation
static int custom_atoi(const char *str) {
    int result = 0;
    int sign = 1;
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Process digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

int parse_http_request(const char *buffer, size_t buffer_size, http_request_t *request) {
    // Initialize the request structure
    memset(request, 0, sizeof(http_request_t));
    
    // Check for null pointers and zero buffer size
    if (!buffer || !request || buffer_size == 0) {
        return -1;
    }
    
    // Find the end of the request line
    const char *end_of_line = strstr(buffer, "\r\n");
    if (!end_of_line) {
        return -1;
    }
    
    // Parse the request line
    size_t line_length = end_of_line - buffer;
    if (parse_request_line(buffer, line_length, request) != 0) {
        return -1;
    }
    
    // Process the headers
    const char *current_pos = end_of_line + 2;  // Skip \r\n
    while (current_pos < buffer + buffer_size) {
        // Check if we've reached the end of headers
        if (current_pos[0] == '\r' && current_pos[1] == '\n') {
            // Skip the empty line after headers
            current_pos += 2;
            break;
        }
        
        // Find the end of the current header line
        const char *header_end = strstr(current_pos, "\r\n");
        if (!header_end) {
            break;
        }
        
        // Find the colon separator
        const char *colon = memchr(current_pos, ':', header_end - current_pos);
        if (colon) {
            // Extract the header name
            size_t name_length = colon - current_pos;
            char header_name[256];
            
            if (name_length < sizeof(header_name)) {
                memcpy(header_name, current_pos, name_length);
                header_name[name_length] = '\0';
                
                // Extract the header value
                const char *value_start = colon + 1;
                while (value_start < header_end && isspace(*value_start)) {
                    value_start++;
                }
                
                size_t value_length = header_end - value_start;
                char header_value[768];
                
                if (value_length < sizeof(header_value)) {
                    memcpy(header_value, value_start, value_length);
                    header_value[value_length] = '\0';
                    
                    // Process common headers
                    if (strcasecmp(header_name, "Host") == 0) {
                        strncpy(request->host, header_value, sizeof(request->host) - 1);
                    } else if (strcasecmp(header_name, "Content-Type") == 0) {
                        strncpy(request->content_type, header_value, sizeof(request->content_type) - 1);
                    } else if (strcasecmp(header_name, "Content-Length") == 0) {
                        request->content_length = custom_atoi(header_value);
                    }
                }
            }
        }
        
        // Move to the next header
        current_pos = header_end + 2;
    }
    
    // Handle the body if present
    if (request->content_length > 0) {
        size_t body_size = buffer_size - (current_pos - buffer);
        if (body_size >= request->content_length) {
            request->body = malloc(request->content_length + 1);
            if (!request->body) {
                return -1;
            }
            
            memcpy(request->body, current_pos, request->content_length);
            request->body[request->content_length] = '\0';
        }
    }
    
    return 0;
}

void free_http_request(http_request_t *request) {
    if (request && request->body) {
        free(request->body);
        request->body = NULL;
    }
}