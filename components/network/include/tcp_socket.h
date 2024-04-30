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

#include "common.h"

#define tcp_TIMEOUT_MS      5000

/**
 * @brief Represents a tcp server.
 */
typedef struct tcp_server_t tcp_server_t;

/**
 * @brief Represents a tcp connection.
 */
typedef struct tcp_connection_t tcp_connection_t;

/**
 * @brief Represents a tcp client.
 */
typedef struct tcp_client_t tcp_client_t;

/**
 * @brief Creates a new tcp server.
 *
 * This function creates a new tcp server that listens on the specified port.
 *
 * @param port The port number on which the server should listen.
 * @return A pointer to the newly created tcp_server_t object.
 */
tcp_server_t *tcp_server_create(uint32_t port);

/**
 * @brief Accepts a new connection on the tcp server.
 *
 * This function accepts a new connection on the tcp server and returns a tcp_connection_t object representing the connection.
 *
 * @param server A pointer to the tcp_server_t object.
 * @return A pointer to the newly created tcp_connection_t object.
 */
tcp_connection_t *tcp_server_accept(tcp_server_t *server);

/**
 * @brief Closes the tcp server.
 *
 * This function closes the tcp server, terminating all active connections.
 *
 * @param server A pointer to the tcp_server_t object to be closed.
 */
void tcp_server_close(tcp_server_t *server);

/**
 * @brief Frees the memory allocated for a tcp server.
 *
 * This function frees the memory allocated for a tcp server object.
 * It should be called when the server is no longer needed to avoid memory leaks.
 *
 * @param server A pointer to the tcp_server_t object to be freed.
 */
void tcp_server_free(tcp_server_t *server);

/**
 * @brief Gets the IP address of the tcp server.
 *
 * This function gets the IP address of the tcp server and stores it in the provided buffer.
 *
 * @param server A pointer to the tcp_server_t object.
 * @param ip A buffer where the IP address will be stored.
 */
void tcp_server_get_ip(tcp_server_t *server, char ip[INET6_ADDRSTRLEN]);

/**
 * @brief Gets the port number of the tcp server.
 *
 * This function gets the port number of the tcp server.
 *
 * @param server A pointer to the tcp_server_t object.
 * @return The port number of the tcp server.
 */
uint32_t tcp_server_get_port(tcp_server_t *server);

/**
 * @brief Checks if a tcp server is closed.
 *
 * This function checks if the specified tcp server is closed.
 *
 * @param server A pointer to the tcp server.
 * @return Returns 1 if the tcp server is closed, 0 otherwise.
 */
uint8_t tcp_server_is_closed(tcp_server_t *server);

void tcp_connection_set_blocking(tcp_connection_t *connection, uint8_t blocking);

/**
 * Sends data over a tcp connection.
 *
 * @param connection The tcp connection to send data through.
 * @param data The data to send.
 * @param data_len The length of the data to send.
 * @return The number of bytes sent, or -1 if an error occurred.
 */
uint32_t tcp_connection_send(tcp_connection_t *connection, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Receives data from a tcp connection.
 *
 * This function receives data from the specified tcp connection and stores it in the provided buffer.
 *
 * @param connection A pointer to the tcp connection.
 * @param buffer A pointer to the buffer where the received data will be stored.
 * @param buffer_len The length of the buffer.
 * @return The number of bytes received, or -1 if an error occurred.
 */
uint32_t tcp_connection_recv(tcp_connection_t *connection, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Closes a tcp connection.
 *
 * This function closes the specified tcp connection.
 *
 * @param connection Pointer to the tcp_connection_t structure representing the connection to be closed.
 */
void tcp_connection_close(tcp_connection_t *connection);

/**
 * @brief Frees the memory allocated for a tcp connection.
 *
 * This function frees the memory allocated for a tcp connection object.
 * It should be called when the connection is no longer needed to avoid memory leaks.
 *
 * @param connection A pointer to the tcp_connection_t object to be freed.
 */
void tcp_connection_free(tcp_connection_t *connection);

/**
 * @brief Checks if a tcp connection is closed.
 *
 * This function checks if the specified tcp connection is closed.
 *
 * @param connection A pointer to the tcp_connection_t structure representing the tcp connection.
 * @return Returns 1 if the tcp connection is closed, 0 otherwise.
 */
uint8_t tcp_connection_is_closed(tcp_connection_t *connection);

/**
 * @brief Represents a tcp client.
 *
 * This structure is used to create and manage a tcp client connection.
 * It provides functions for creating a tcp client and connecting to a server.
 * 
 * @param host_ip The IP address of the server to connect to.
 * @param port The port number of the server to connect to.
 */
tcp_client_t *tcp_client_create(const char *host_ip, uint32_t port);

void tcp_client_set_blocking(tcp_client_t *client, uint8_t blocking);

/**
 * Sends data over a tcp client.
 *
 * @param client    The tcp client to send data through.
 * @param data      The data to send.
 * @param data_len  The length of the data to send.
 *
 * @return          The number of bytes sent, or -1 if an error occurred.
 */
uint32_t tcp_client_send(tcp_client_t *client, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Receives data from the tcp client.
 *
 * This function receives data from the specified tcp client and stores it in the provided buffer.
 *
 * @param client The tcp client to receive data from.
 * @param buffer The buffer to store the received data.
 * @param buffer_len The length of the buffer.
 * @return The number of bytes received, or -1 if an error occurred.
 */
uint32_t tcp_client_recv(tcp_client_t *client, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Closes the tcp client connection.
 *
 * This function closes the connection of the specified tcp client.
 *
 * @param client A pointer to the tcp_client_t structure representing the tcp client.
 */
void tcp_client_close(tcp_client_t *client);

/**
 * @brief Frees the resources associated with a tcp client.
 *
 * This function releases the memory allocated for the tcp client object and frees any resources
 * associated with it. After calling this function, the tcp client object should no longer be used.
 *
 * @param client A pointer to the tcp client object to be freed.
 */
void tcp_client_free(tcp_client_t *client);

/**
 * @brief Checks if a tcp client is closed.
 *
 * This function checks if the specified tcp client is closed or not.
 *
 * @param client A pointer to the tcp client structure.
 * @return Returns 1 if the tcp client is closed, 0 otherwise.
 */
uint8_t tcp_client_is_closed(tcp_client_t *client);