
CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -pthread

SOURCES = echo_server.c client_handler.c utils.c http_request.c http_response.c route_handler.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = http_server

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

# Dependencies
echo_server.o: echo_server.c echo_server.h client_handler.h utils.h
client_handler.o: client_handler.c client_handler.h echo_server.h http_request.h http_response.h route_handler.h
utils.o: utils.c utils.h
http_request.o: http_request.c http_request.h
http_response.o: http_response.c http_response.h
route_handler.o: route_handler.c route_handler.h http_request.h http_response.h

.PHONY: all clean