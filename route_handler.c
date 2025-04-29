#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "route_handler.h"

// Base directory for static files
#define STATIC_DIR "./static"
#define MAX_FILE_SIZE (10 * 1024 * 1024) // 10MB max

// Custom atoi implementation
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

//check if a string is a number
static int is_number(const char *str) {
    if (!str || *str == '\0') {
        return 0;
    }
    
    // Check for negative sign
    if (*str == '-') {
        str++;
    }
    
    // Must have at least one digit
    if (*str == '\0') {
        return 0;
    }
    
    // Check that all characters are digits
    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    
    return 1;
}

//extract path components
static int extract_path_components(const char *path, char **components, int max_components) {
    if (!path || !components || max_components <= 0) {
        return 0;
    }
    
    char *path_copy = strdup(path);
    if (!path_copy) {
        return 0;
    }
    
    int count = 0;
    
    // Skip leading slash
    char *p = path_copy;
    if (*p == '/') {
        p++;
    }
    
    // Tokenize the path
    char *token = p;
    char *slash = NULL;
    
    while (token && count < max_components) {
        slash = strchr(token, '/');
        if (slash) {
            *slash = '\0';
            components[count++] = strdup(token);
            token = slash + 1;
        } else {
            if (*token != '\0') {
                components[count++] = strdup(token);
            }
            break;
        }
    }
    
    free(path_copy);
    return count;
}

// Handler for incoming requests
void handle_request(const http_request_t *request, http_response_t *response) {
    if (!request || !response) {
        set_response_status(response, HTTP_STATUS_INTERNAL_ERROR);
        set_response_body_string(response, "Internal Server Error");
        return;
    }
    
    // Check if the method is GET
    if (strcmp(request->method, "GET") != 0) {
        set_response_status(response, HTTP_STATUS_METHOD_NOT_ALLOWED);
        set_response_body_string(response, "Method Not Allowed. Only GET is supported.");
        return;
    }
    
    // Route based on the path
    if (strncmp(request->path, "/static/", 8) == 0) {
        handle_static_file(request->path + 7, response); // +7 to skip "/static"
    } else if (strncmp(request->path, "/calc/", 6) == 0) {
        handle_calc(request->path, response);
    } else if (strncmp(request->path, "/sleep/", 7) == 0) {
        handle_sleep(request->path, response);
    } else if (strcmp(request->path, "/") == 0 || strcmp(request->path, "/index.html") == 0) {
        //simple welcome page
        set_response_content_type(response, "text/html");
        set_response_body_string(response, 
            "<html>"
            "<head><title>HTTP Server</title></head>"
            "<body>"
            "<h1>Welcome to the HTTP Server</h1>"
            "<p>Available routes:</p>"
            "<ul>"
            "<li>/static/[filename] - Serves static files</li>"
            "<li>/calc/add/[num1]/[num2] - Addition</li>"
            "<li>/calc/mul/[num1]/[num2] - Multiplication</li>"
            "<li>/calc/div/[num1]/[num2] - Division</li>"
            "<li>/sleep/[seconds] - Sleep for testing pipelining</li>"
            "</ul>"
            "</body>"
            "</html>"
        );
    } else {
        set_response_status(response, HTTP_STATUS_NOT_FOUND);
        set_response_content_type(response, "text/html");
        set_response_body_string(response, 
            "<html>"
            "<head><title>404 Not Found</title></head>"
            "<body>"
            "<h1>404 Not Found</h1>"
            "<p>The requested resource was not found on this server.</p>"
            "</body>"
            "</html>"
        );
    }
}

void handle_static_file(const char *path, http_response_t *response) {
    if (!path || !response) {
        set_response_status(response, HTTP_STATUS_INTERNAL_ERROR);
        return;
    }
    
    // Ensure path doesn't contain ".." to prevent directory traversal
    if (strstr(path, "..")) {
        set_response_status(response, HTTP_STATUS_BAD_REQUEST);
        set_response_body_string(response, "Bad Request: Invalid path");
        return;
    }
    
    // Construct the full path
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s", STATIC_DIR, path);
    
    // Open the file
    int fd = open(full_path, O_RDONLY);
    if (fd < 0) {
        set_response_status(response, HTTP_STATUS_NOT_FOUND);
        set_response_body_string(response, "File not found");
        return;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        set_response_status(response, HTTP_STATUS_NOT_FOUND);
        set_response_body_string(response, "Not a regular file");
        return;
    }
    
    // Check if file is too large
    if (st.st_size > MAX_FILE_SIZE) {
        close(fd);
        set_response_status(response, HTTP_STATUS_INTERNAL_ERROR);
        set_response_body_string(response, "File too large");
        return;
    }
    
    // Read the file
    void *file_buffer = malloc(st.st_size);
    if (!file_buffer) {
        close(fd);
        set_response_status(response, HTTP_STATUS_INTERNAL_ERROR);
        set_response_body_string(response, "Server error: Failed to allocate memory");
        return;
    }
    
    ssize_t bytes_read = read(fd, file_buffer, st.st_size);
    close(fd);
    
    if (bytes_read != st.st_size) {
        free(file_buffer);
        set_response_status(response, HTTP_STATUS_INTERNAL_ERROR);
        set_response_body_string(response, "Server error: Failed to read file");
        return;
    }
    
    // Set the content type based on file extension
    set_response_content_type(response, get_content_type_for_file(full_path));
    
    // Set the response body
    set_response_body(response, file_buffer, bytes_read);
    
    // Free the file buffer (now copied to response)
    free(file_buffer);
}

void handle_calc(const char *path, http_response_t *response) {
    if (!path || !response) {
        set_response_status(response, HTTP_STATUS_INTERNAL_ERROR);
        return;
    }
    
    // Parse the path components
    // Expected format: /calc/operation/num1/num2
    char *components[10] = {0};
    int component_count = extract_path_components(path, components, 10);
    
    if (component_count < 4 || strcmp(components[0], "calc") != 0) {
        set_response_status(response, HTTP_STATUS_BAD_REQUEST);
        set_response_body_string(response, "Bad Request: Invalid calculator path");
        goto cleanup;
    }
    
    // Get the operation and numbers
    char *operation = components[1];
    char *num1_str = components[2];
    char *num2_str = components[3];
    
    // Validate the numbers
    if (!is_number(num1_str) || !is_number(num2_str)) {
        set_response_status(response, HTTP_STATUS_BAD_REQUEST);
        set_response_body_string(response, "Bad Request: Invalid number format");
        goto cleanup;
    }
    
    int num1 = custom_atoi(num1_str);
    int num2 = custom_atoi(num2_str);
    
    // Perform the calculation
    char result_buffer[256];
    if (strcmp(operation, "add") == 0) {
        snprintf(result_buffer, sizeof(result_buffer), "%d + %d = %d", num1, num2, num1 + num2);
    } else if (strcmp(operation, "mul") == 0) {
        snprintf(result_buffer, sizeof(result_buffer), "%d * %d = %d", num1, num2, num1 * num2);
    } else if (strcmp(operation, "div") == 0) {
        if (num2 == 0) {
            set_response_status(response, HTTP_STATUS_BAD_REQUEST);
            set_response_body_string(response, "Error: Division by zero");
            goto cleanup;
        }
        snprintf(result_buffer, sizeof(result_buffer), "%d / %d = %d", num1, num2, num1 / num2);
    } else {
        set_response_status(response, HTTP_STATUS_BAD_REQUEST);
        set_response_body_string(response, "Bad Request: Unknown operation");
        goto cleanup;
    }
    
    // Set the response
    set_response_content_type(response, "text/html");
    
    char html_response[1024];
    snprintf(html_response, sizeof(html_response),
        "<html>"
        "<head><title>Calculator Result</title></head>"
        "<body>"
        "<h1>Calculator Result</h1>"
        "<p>%s</p>"
        "<p><a href='/'>Back to home</a></p>"
        "</body>"
        "</html>",
        result_buffer
    );
    
    set_response_body_string(response, html_response);
    
cleanup:
    // Free the components
    for (int i = 0; i < component_count; i++) {
        free(components[i]);
    }
}

void handle_sleep(const char *path, http_response_t *response) {
    if (!path || !response) {
        set_response_status(response, HTTP_STATUS_INTERNAL_ERROR);
        return;
    }
    
    // Parse the path components
    // Expected format: /sleep/seconds
    char *components[10] = {0};
    int component_count = extract_path_components(path, components, 10);
    
    if (component_count < 2 || strcmp(components[0], "sleep") != 0) {
        set_response_status(response, HTTP_STATUS_BAD_REQUEST);
        set_response_body_string(response, "Bad Request: Invalid sleep path");
        goto cleanup;
    }
    
    // Get the sleep duration
    char *seconds_str = components[1];
    
    // Validate the number
    if (!is_number(seconds_str)) {
        set_response_status(response, HTTP_STATUS_BAD_REQUEST);
        set_response_body_string(response, "Bad Request: Invalid number format");
        goto cleanup;
    }
    
    int seconds = custom_atoi(seconds_str);
    
    // Limit the sleep duration to a reasonable value
    if (seconds > 10) {
        seconds = 10;
    }
    
    // Sleep for the specified duration
    sleep(seconds);
    
    // Set the response
    set_response_content_type(response, "text/html");
    
    char html_response[1024];
    snprintf(html_response, sizeof(html_response),
        "<html>"
        "<head><title>Sleep Result</title></head>"
        "<body>"
        "<h1>Sleep Complete</h1>"
        "<p>Slept for %d seconds.</p>"
        "<p><a href='/'>Back to home</a></p>"
        "</body>"
        "</html>",
        seconds
    );
    
    set_response_body_string(response, html_response);
    
cleanup:
    // Free the components
    for (int i = 0; i < component_count; i++) {
        free(components[i]);
    }
}