// client_handler.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "client_handler.h"
#include "echo_server.h"
#include "http_request.h"
#include "http_response.h"
#include "route_handler.h"

// Define buffer for HTTP responses
#define HTTP_BUFFER_SIZE (10 * 1024 * 1024) // 10MB max response size

// Function that handles client connections via different threads
void* handle_client_connection(void* arg) {
    client_connection_t* client_info = (client_connection_t*)arg;
    int client_socket = client_info->client_socket;
    struct sockaddr_in client_address = client_info->client_address;
    free(client_info);  // Free the allocated structure
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_address.sin_port);
    
    if (verbose_mode) {
        printf("Connection established with %s:%d\n", client_ip, client_port);
    }
    
    // Buffer for receiving HTTP request
    char *http_buffer = malloc(BUFFER_SIZE);
    if (!http_buffer) {
        if (verbose_mode) {
            printf("Failed to allocate memory for HTTP buffer\n");
        }
        close(client_socket);
        pthread_exit(NULL);
    }
    
    // Buffer for sending HTTP response
    char *response_buffer = malloc(HTTP_BUFFER_SIZE);
    if (!response_buffer) {
        if (verbose_mode) {
            printf("Failed to allocate memory for response buffer\n");
        }
        free(http_buffer);
        close(client_socket);
        pthread_exit(NULL);
    }
    
    // Read data from client
    ssize_t bytes_read = 0;
    size_t total_bytes = 0;
    
    // Read until we have the complete HTTP request
    while (1) {
        bytes_read = recv(client_socket, http_buffer + total_bytes, BUFFER_SIZE - total_bytes - 1, 0);
        
        if (bytes_read <= 0) {
            if (verbose_mode) {
                if (bytes_read == 0) {
                    printf("Connection with %s:%d closed by client\n", client_ip, client_port);
                } else {
                    printf("Error reading from client %s:%d: %s\n", 
                           client_ip, client_port, strerror(errno));
                }
            }
            free(http_buffer);
            free(response_buffer);
            close(client_socket);
            pthread_exit(NULL);
        }
        
        total_bytes += bytes_read;
        http_buffer[total_bytes] = '\0';
        
        // Check if we've received the end of the HTTP headers
        if (strstr(http_buffer, "\r\n\r\n") != NULL) {
            break;
        }
        
        // Check if buffer is full
        if (total_bytes >= BUFFER_SIZE - 1) {
            break;
        }
    }
    
    if (verbose_mode) {
        printf("Received HTTP request from %s:%d:\n%s\n", client_ip, client_port, http_buffer);
    }
    
    // Parse the HTTP request
    http_request_t request;
    if (parse_http_request(http_buffer, total_bytes, &request) != 0) {
        // Invalid request format
        const char *error_response = 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Invalid request";
        
        send(client_socket, error_response, strlen(error_response), 0);
        free(http_buffer);
        free(response_buffer);
        close(client_socket);
        pthread_exit(NULL);
    }
    
    // Process the request and generate a response
    http_response_t response;
    init_http_response(&response);
    
    handle_request(&request, &response);
    
    // Write the response to the buffer
    size_t response_size = write_http_response(&response, response_buffer, HTTP_BUFFER_SIZE);
    
    // Send the response
    if (response_size > 0) {
        ssize_t bytes_sent = send(client_socket, response_buffer, response_size, 0);
        if (bytes_sent < 0) {
            if (verbose_mode) {
                printf("Error sending response to client %s:%d: %s\n", 
                       client_ip, client_port, strerror(errno));
            }
        } else if (verbose_mode) {
            printf("Sent HTTP response to %s:%d (%zu bytes)\n", client_ip, client_port, bytes_sent);
        }
    }
    
    // Clean up
    free_http_request(&request);
    free_http_response(&response);
    free(http_buffer);
    free(response_buffer);
    
    // Close the client socket
    close(client_socket);
    if (verbose_mode) {
        printf("Connection with %s:%d closed\n", client_ip, client_port);
    }
    
    pthread_exit(NULL);
}