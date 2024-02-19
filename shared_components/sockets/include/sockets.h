#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <stdio.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

typedef int server_socket_t;
typedef int connection_socket_t;
typedef int client_socket_t;

int socket_server_init(int port);
connection_socket_t socket_server_accept(server_socket_t server);
int socket_server_close(server_socket_t server);

int socket_connection_send(connection_socket_t connection, const char *data, int data_len);
int socket_connection_recv(connection_socket_t connection, char *buffer, int buffer_len);
int socket_connection_close(connection_socket_t connection);

int socket_client_init(const char *host_ip, int port);
int socket_client_send(client_socket_t client, const char *data, int data_len);
int socket_client_recv(client_socket_t client, char *buffer, int buffer_len);
int socket_client_close(client_socket_t client);

#endif