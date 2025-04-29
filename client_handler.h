#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "echo_server.h"

// Handle client connections in separate threads
void* handle_client_connection(void* arg);

#endif