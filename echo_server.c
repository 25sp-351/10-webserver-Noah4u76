#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "echo_server.h"
#include "client_handler.h"
#include "utils.h"
#include <sys/stat.h>

// Global variables
int server_socket = -1;
int verbose_mode = 0;

// Function to handle ctrl+c
void handle_interrupt_signal(int signal_number) {
    (void)signal_number; // Unused parameter, avoid compiler warning
    
    if (server_socket != -1) {
        close(server_socket);
    }
    printf("\nServer shutting down...\n");
    exit(EXIT_SUCCESS);
}

// Initialize server to port number
int initialize_server(int server_port) {
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        return -1;
    }
    
    // Set socket options to allow address reuse
    int reuse_address_flag = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_address_flag, sizeof(reuse_address_flag)) < 0) {
        perror("Failed to set socket options");
        close(server_socket);
        return -1;
    }
    
    // Set up server address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);
    
    // Bind socket to address
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Failed to bind socket");
        close(server_socket);
        return -1;
    }
    
    // Listen for connections
    if (listen(server_socket, MAX_PENDING_CONNECTIONS) < 0) {
        perror("Failed to listen");
        close(server_socket);
        return -1;
    }
    
    printf("HTTP server listening on port %d\n", server_port);
    return 0;
}

// Main server loop to accept and handle client connections
void run_server_loop(void) {
    // Create 'static' directory if it doesn't exist
    if (access("./static", F_OK) != 0) {
        if (mkdir("./static", 0755) != 0) {
            perror("Warning: Failed to create 'static' directory");
        } else {
            printf("Created 'static' directory for serving files\n");
        }
    }
    
    // Main loop to accept connections
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        
        // Accept a new client connection
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);
        
        if (client_socket < 0) {
            perror("Failed to accept connection");
            continue;  // Continue to next iteration to accept new connections
        }
        
        // Allocate memory for client info
        client_connection_t* client_info = malloc(sizeof(client_connection_t));
        if (!client_info) {
            perror("Failed to allocate memory");
            close(client_socket);
            continue;
        }
        
        client_info->client_socket = client_socket;
        client_info->client_address = client_address;
        
        // Create a new thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_connection, (void*)client_info) != 0) {
            perror("Failed to create thread");
            free(client_info);
            close(client_socket);
            continue;
        }
        
        // Detach the thread so its resources are automatically released when it terminates
        pthread_detach(thread_id);
    }
}

int main(int argc, char* argv[]) {
    int server_port = DEFAULT_PORT;
    int option;
    
    // Parse command line arguments
    while ((option = getopt(argc, argv, "p:v")) != -1) {
        switch (option) {
            case 'p':
                server_port = string_to_int(optarg);
                if (server_port <= 0 || server_port > 65535) {
                    fprintf(stderr, "Invalid port number. Using default port %d.\n", DEFAULT_PORT);
                    server_port = DEFAULT_PORT;
                }
                break;
            case 'v':
                verbose_mode = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-p port] [-v]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    // Set up signal handler for Ctrl+C
    signal(SIGINT, handle_interrupt_signal);
    
    // Initialize server
    if (initialize_server(server_port) < 0) {
        exit(EXIT_FAILURE);
    }
    
    printf("HTTP Server started. Available endpoints:\n");
    printf("  /                       - Home page\n");
    printf("  /static/[filename]      - Access static files\n");
    printf("  /calc/add/[num1]/[num2] - Addition\n");
    printf("  /calc/mul/[num1]/[num2] - Multiplication\n");
    printf("  /calc/div/[num1]/[num2] - Division\n");
    printf("  /sleep/[seconds]        - Sleep (for testing pipelining)\n");
    
    // Run the server main loop
    run_server_loop();
    
    // Close the server socket (though we should never reach this point)
    close(server_socket);
    
    return 0;
}