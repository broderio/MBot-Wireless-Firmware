#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SOCKET_TIMEOUT_MS      5000

/**
 * @brief Represents a socket server.
 */
typedef struct socket_server_t socket_server_t;

/**
 * @brief Represents a socket connection.
 */
typedef struct socket_connection_t socket_connection_t;

/**
 * @brief Represents a socket client.
 */
typedef struct socket_client_t socket_client_t;

/**
 * @brief Creates a new socket server.
 *
 * This function creates a new socket server that listens on the specified port.
 *
 * @param port The port number on which the server should listen.
 * @return A pointer to the newly created socket_server_t object.
 */
socket_server_t *socket_server_create(uint32_t port);

/**
 * @brief Accepts a new connection on the socket server.
 *
 * This function accepts a new connection on the socket server and returns a socket_connection_t object representing the connection.
 *
 * @param server A pointer to the socket_server_t object.
 * @return A pointer to the newly created socket_connection_t object.
 */
socket_connection_t *socket_server_accept(socket_server_t *server);

/**
 * @brief Closes the socket server.
 *
 * This function closes the socket server, terminating all active connections.
 *
 * @param server A pointer to the socket_server_t object to be closed.
 */
void socket_server_close(socket_server_t *server);

/**
 * @brief Frees the memory allocated for a socket server.
 *
 * This function frees the memory allocated for a socket server object.
 * It should be called when the server is no longer needed to avoid memory leaks.
 *
 * @param server A pointer to the socket_server_t object to be freed.
 */
void socket_server_free(socket_server_t *server);

/**
 * @brief Gets the IP address of the socket server.
 *
 * This function gets the IP address of the socket server and stores it in the provided buffer.
 *
 * @param server A pointer to the socket_server_t object.
 * @param ip A buffer where the IP address will be stored.
 */
void socket_server_get_ip(socket_server_t *server, char ip[INET6_ADDRSTRLEN]);

/**
 * @brief Gets the port number of the socket server.
 *
 * This function gets the port number of the socket server.
 *
 * @param server A pointer to the socket_server_t object.
 * @return The port number of the socket server.
 */
uint32_t socket_server_get_port(socket_server_t *server);

/**
 * @brief Checks if a socket server is closed.
 *
 * This function checks if the specified socket server is closed.
 *
 * @param server A pointer to the socket server.
 * @return Returns 1 if the socket server is closed, 0 otherwise.
 */
uint8_t socket_server_is_closed(socket_server_t *server);

/**
 * Sends data over a socket connection.
 *
 * @param connection The socket connection to send data through.
 * @param data The data to send.
 * @param data_len The length of the data to send.
 * @return The number of bytes sent, or -1 if an error occurred.
 */
uint32_t socket_connection_send(socket_connection_t *connection, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Receives data from a socket connection.
 *
 * This function receives data from the specified socket connection and stores it in the provided buffer.
 *
 * @param connection A pointer to the socket connection.
 * @param buffer A pointer to the buffer where the received data will be stored.
 * @param buffer_len The length of the buffer.
 * @return The number of bytes received, or -1 if an error occurred.
 */
uint32_t socket_connection_recv(socket_connection_t *connection, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Closes a socket connection.
 *
 * This function closes the specified socket connection.
 *
 * @param connection Pointer to the socket_connection_t structure representing the connection to be closed.
 */
void socket_connection_close(socket_connection_t *connection);

/**
 * @brief Frees the memory allocated for a socket connection.
 *
 * This function frees the memory allocated for a socket connection object.
 * It should be called when the connection is no longer needed to avoid memory leaks.
 *
 * @param connection A pointer to the socket_connection_t object to be freed.
 */
void socket_connection_free(socket_connection_t *connection);

/**
 * @brief Checks if a socket connection is closed.
 *
 * This function checks if the specified socket connection is closed.
 *
 * @param connection A pointer to the socket_connection_t structure representing the socket connection.
 * @return Returns 1 if the socket connection is closed, 0 otherwise.
 */
uint8_t socket_connection_is_closed(socket_connection_t *connection);

/**
 * @brief Represents a socket client.
 *
 * This structure is used to create and manage a socket client connection.
 * It provides functions for creating a socket client and connecting to a server.
 */
socket_client_t *socket_client_create(const char *host_ip, uint32_t port);

/**
 * Sends data over a socket client.
 *
 * @param client    The socket client to send data through.
 * @param data      The data to send.
 * @param data_len  The length of the data to send.
 *
 * @return          The number of bytes sent, or -1 if an error occurred.
 */
uint32_t socket_client_send(socket_client_t *client, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Receives data from the socket client.
 *
 * This function receives data from the specified socket client and stores it in the provided buffer.
 *
 * @param client The socket client to receive data from.
 * @param buffer The buffer to store the received data.
 * @param buffer_len The length of the buffer.
 * @return The number of bytes received, or -1 if an error occurred.
 */
uint32_t socket_client_recv(socket_client_t *client, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Closes the socket client connection.
 *
 * This function closes the connection of the specified socket client.
 *
 * @param client A pointer to the socket_client_t structure representing the socket client.
 */
void socket_client_close(socket_client_t *client);

/**
 * @brief Frees the resources associated with a socket client.
 *
 * This function releases the memory allocated for the socket client object and frees any resources
 * associated with it. After calling this function, the socket client object should no longer be used.
 *
 * @param client A pointer to the socket client object to be freed.
 */
void socket_client_free(socket_client_t *client);

/**
 * @brief Checks if a socket client is closed.
 *
 * This function checks if the specified socket client is closed or not.
 *
 * @param client A pointer to the socket client structure.
 * @return Returns 1 if the socket client is closed, 0 otherwise.
 */
uint8_t socket_client_is_closed(socket_client_t *client);