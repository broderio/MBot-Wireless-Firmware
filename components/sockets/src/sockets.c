#include <stdio.h>
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "sockets.h"

const char *SOCKET_TAG = "sockets";

typedef struct socket_t {
    int32_t _fd;
    int32_t _port;
    uint8_t _closed;
} socket_t;

struct socket_server_t 
{   socket_t _socket;
};

struct socket_connection_t 
{   socket_t _socket;
    uint32_t _last_recv_time;
};

struct socket_client_t 
{   socket_t _socket;
    uint32_t _last_recv_time;
};

uint32_t _get_time_ms_socket() {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

/**
 * @brief Sends data over a socket.
 *
 * This function sends the specified buffer of data over the socket with the given file descriptor.
 * It sends the data in chunks until the entire buffer is sent.
 *
 * @param fd The file descriptor of the socket.
 * @param buffer Pointer to the buffer containing the data to be sent.
 * @param buffer_len The length of the buffer in bytes.
 * @return The number of bytes sent on success, or -1 on failure.
 */
int64_t _socket_send(int32_t fd, uint8_t *buffer, uint32_t buffer_len)
{
    int64_t bytes_sent = 0;
    while (bytes_sent < buffer_len) {
        int64_t bytes = send(fd, buffer + bytes_sent, buffer_len - bytes_sent, 0);
        if (bytes < 0) {
            ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            return -1;
        }
        bytes_sent += bytes;
    }
    return bytes_sent;
}

/**
 * @brief Receives data from a socket.
 *
 * This function receives data from the socket with the given file descriptor and stores it in the specified buffer.
 * It returns the number of bytes received.
 *
 * @param fd The file descriptor of the socket.
 * @param buffer Pointer to the buffer where the received data will be stored.
 * @param buffer_len The length of the buffer in bytes.
 * @return The number of bytes received on success, 0 if no data is available, -1 on failure, or -2 if the socket has been disconnected.
 */
int64_t _socket_recv(int32_t fd, uint8_t *buffer, uint32_t buffer_len)
{
    int64_t len = recv(fd, buffer, buffer_len, 0);
    if (len < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // Not an error
        }
        else if (errno == ENOTCONN) {
            ESP_LOGW(SOCKET_TAG, "Connection closed");
            return -2;  // Socket has been disconnected
        }
        ESP_LOGE(SOCKET_TAG, "Error occurred during receiving: errno %d", errno);
        return -1;
    }

    return len;
}

/**
 * @brief Closes a socket.
 *
 * This function closes the socket associated with the given socket object.
 *
 * @param socket Pointer to the socket object.
 */
void _socket_close(socket_t *socket)
{
    close(socket->_fd);
    socket->_closed = 1;
}

socket_server_t *socket_server_create(uint32_t port)
{
    socket_server_t *server = (socket_server_t *)malloc(sizeof(socket_server_t));
    if (server == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to allocate memory for server");
        return NULL;
    }
    server->_socket._port = port;
    server->_socket._closed = 1;
    char port_str[6];
    snprintf(port_str, 6, "%lu", port);

    // Set the server address and port
    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;

    int res = getaddrinfo("0.0.0.0", port_str, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to resolve hostname for 0.0.0.0 getaddrinfo() returns %d, addrinfo=%p", res, address_info);
        socket_server_free(server);
        return NULL;
    }

    // Create a socket
    server->_socket._fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (server->_socket._fd < 0) {
        ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
        socket_server_free(server);
        return NULL;
    }
    server->_socket._closed = 0;

    // Set the reuse address option
    int opt = 1;
    setsockopt(server->_socket._fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server->_socket._fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    // Set the socket to non-blocking
    int flags = fcntl(server->_socket._fd, F_GETFL);
    if (fcntl(server->_socket._fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        ESP_LOGE(SOCKET_TAG, "Unable to set socket non blocking: errno %d", errno);
        socket_server_free(server);
        return NULL;
    }

    ESP_LOGI(SOCKET_TAG, "Socket created");

    // Bind the socket to the server
    int err = bind(server->_socket._fd, address_info->ai_addr, address_info->ai_addrlen);
    if (err != 0) {
        ESP_LOGE(SOCKET_TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(SOCKET_TAG, "IPPROTO: %d", AF_INET);
        socket_server_free(server);
        return NULL;
    }
    ESP_LOGI(SOCKET_TAG, "Socket bound, port %lu", port);
 
    // Start listening
    err = listen(server->_socket._fd, 1);
    if (err != 0) {
        ESP_LOGE(SOCKET_TAG, "Error occurred during listen: errno %d", errno);
        socket_server_free(server);
        return NULL;
    }
    return server;
}

socket_connection_t *socket_server_accept(socket_server_t *server)
{
    if (server == NULL) {
        return NULL;
    }
    
    socket_connection_t *connection = (socket_connection_t *)malloc(sizeof(socket_connection_t));
    if (connection == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to allocate memory for connection");
        return NULL;
    }

    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);
    connection->_socket._closed = 1;
    connection->_socket._fd = accept(server->_socket._fd, (struct sockaddr *)&source_addr, &addr_len);

    if (connection->_socket._fd < 0) {
        if (errno == EWOULDBLOCK) 
        {
            ESP_LOGV(SOCKET_TAG, "No pending connections...");
            return NULL;
        }
        ESP_LOGE(SOCKET_TAG, "Unable to accept connection: errno %s", esp_err_to_name(errno));
        return NULL;
    }
    connection->_socket._closed = 0;

    int flags = fcntl(connection->_socket._fd, F_GETFL);
    if (fcntl(connection->_socket._fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        ESP_LOGE(SOCKET_TAG, "Unable to set socket non blocking: errno %d", errno);
        socket_connection_free(connection);
        return NULL;
    }

    ESP_LOGI(SOCKET_TAG, "Socket accepted!");
    connection->_last_recv_time = _get_time_ms_socket();
    return connection;
}

void socket_server_close(socket_server_t *server)
{
    if (server == NULL) {
        return;
    }
    _socket_close(&server->_socket);
}

void socket_server_free(socket_server_t *server)
{
    if (server == NULL) {
        return;
    }

    if (server->_socket._closed == 0) {
        socket_server_close(server);
    }

    free(server);
}

void socket_server_get_ip(socket_server_t *server, char ip[INET6_ADDRSTRLEN])
{
    if (server == NULL) {
        return;
    }
    struct sockaddr_in6 addr;
    socklen_t addr_len = sizeof(addr);
    getsockname(server->_socket._fd, (struct sockaddr *)&addr, &addr_len);
    inet_ntop(AF_INET6, &addr.sin6_addr, ip, INET6_ADDRSTRLEN);
}

uint32_t socket_server_get_port(socket_server_t *server)
{
    if (server == NULL) {
        return 0;
    }
    return server->_socket._port;
}

uint8_t socket_server_is_closed(socket_server_t *server)
{
    if (server == NULL) {
        return 1;
    }
    return server->_socket._closed;
}

uint32_t socket_connection_send(socket_connection_t *connection, uint8_t *buffer, uint32_t buffer_len)
{
    if (connection == NULL) {
        return 0;
    }

    if (connection->_socket._closed) {
        return 0;
    }

    if (_get_time_ms_socket() - connection->_last_recv_time > SOCKET_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        socket_connection_close(connection);
        return 0;
    }

    int64_t bytes_sent = _socket_send(connection->_socket._fd, buffer, buffer_len);
    if (bytes_sent < 0) {
        socket_connection_close(connection);
        return 0;
    }
    return (uint32_t)bytes_sent;
}

uint32_t socket_connection_recv(socket_connection_t *connection, uint8_t *buffer, uint32_t buffer_len)
{
    if (connection == NULL) {
        return 0;
    }

    if (connection->_socket._closed) {
        return 0;
    }

    if (_get_time_ms_socket() - connection->_last_recv_time > SOCKET_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        socket_connection_close(connection);
        return 0;
    }

    int64_t bytes_read = _socket_recv(connection->_socket._fd, buffer, buffer_len);
    if (bytes_read < 0) {
        socket_connection_close(connection);
        return 0;
    }
    else if (bytes_read == 0) {
        return 0;
    }
    connection->_last_recv_time = _get_time_ms_socket();
    return (uint32_t)bytes_read;
}

void socket_connection_close(socket_connection_t *connection)
{
    if (connection == NULL) {
        return;
    }
    _socket_close(&connection->_socket);
}

uint8_t socket_connection_is_closed(socket_connection_t *connection)
{
    if (connection == NULL) {
        return 1;
    }
    return connection->_socket._closed;
}

void socket_connection_free(socket_connection_t *connection)
{
    if (connection == NULL) {
        return;
    }

    if (connection->_socket._closed == 0) {
        socket_connection_close(connection);
    }

    free(connection);
}

socket_client_t *socket_client_create(const char *host_ip, uint32_t port)
{
    socket_client_t *client = (socket_client_t *)malloc(sizeof(socket_client_t));
    if (client == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to allocate memory for client");
        return NULL;
    }
    client->_socket._port = port;
    client->_socket._closed = 1;

    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;

    char port_str[6];
    snprintf(port_str, 6, "%lu", port);
    int res = getaddrinfo(host_ip, port_str, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to resolve hostname for `%s` getaddrinfo() returns %d, addrinfo=%p", host_ip, res, address_info);
        goto error;
    }

    client->_socket._fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (client->_socket._fd < 0) {
        ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
        goto error;
    }
    client->_socket._closed = 0;

    int flags = fcntl(client->_socket._fd, F_GETFL);
    if (fcntl(client->_socket._fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        ESP_LOGE(SOCKET_TAG, "Unable to set socket non blocking: errno %d", errno);
        goto error;
    }

    ESP_LOGI(SOCKET_TAG, "Socket created, connecting to %s:%lu", host_ip, port);

    // Connecting to the server
    if (connect(client->_socket._fd, address_info->ai_addr, address_info->ai_addrlen)) {
        if (errno == EINPROGRESS) {
            struct pollfd pfd;
            pfd.fd = client->_socket._fd;
            pfd.events = POLLOUT;

            res = poll(&pfd, 1, -1); // -1 means no timeout
            if (res < 0) {
                ESP_LOGE(SOCKET_TAG, "Error during connection: poll for socket to be writable");
                goto error;
            } else if (res == 0) {
                ESP_LOGE(SOCKET_TAG, "Connection timeout: poll for socket to be writable");
                goto error;
            } else {
                int sockerr;
                socklen_t len = (socklen_t)sizeof(int);
                if (getsockopt(client->_socket._fd, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len) < 0) {
                    ESP_LOGE(SOCKET_TAG, "Error when getting socket error using getsockopt()");
                    goto error;
                }
                if (sockerr) {
                    ESP_LOGE(SOCKET_TAG, "Connection error");
                    goto error;
                }
            }
        } else {
            ESP_LOGE(SOCKET_TAG, "Socket is unable to connect");
            goto error;
        }
    }
    ESP_LOGI(SOCKET_TAG, "Successfully connected");
    free(address_info);
    client->_last_recv_time = _get_time_ms_socket();
    return client;

    error:
        socket_client_free(client);
        free(address_info);
        return NULL;
}

uint32_t socket_client_send(socket_client_t *client, uint8_t *buffer, uint32_t buffer_len)
{
    if (client == NULL) {
        return 0;
    }

    if (client->_socket._closed) {
        return 0;
    }
    
    if (_get_time_ms_socket() - client->_last_recv_time > SOCKET_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        socket_client_close(client);
        return 0;
    }
    
    int64_t bytes_sent = _socket_send(client->_socket._fd, buffer, buffer_len);
    if (bytes_sent < 0) {
        socket_client_close(client);
        return 0;
    }
    return (uint32_t)bytes_sent;
}

uint32_t socket_client_recv(socket_client_t *client, uint8_t *buffer, uint32_t buffer_len)
{
    if (client == NULL) {
        return 0;
    }

    if (client->_socket._closed) {
        return 0;
    }
    
    if (_get_time_ms_socket() - client->_last_recv_time > SOCKET_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        socket_client_close(client);
        return 0;
    }
    
    int64_t bytes_read = _socket_recv(client->_socket._fd, buffer, buffer_len);
    if (bytes_read < 0) {
        socket_client_close(client);
        return 0;
    }
    else if (bytes_read == 0) {
        return 0;
    }
    client->_last_recv_time = _get_time_ms_socket();
    return (uint32_t)bytes_read;
}

void socket_client_close(socket_client_t *client)
{
    if (client == NULL) {
        return;
    }
    _socket_close(&client->_socket);
}

void socket_client_free(socket_client_t *client)
{
    if (client == NULL) {
        return;
    }

    if (client->_socket._closed == 0) {
        socket_client_close(client);
    }

    free(client);
}

uint8_t socket_client_is_closed(socket_client_t *client)
{
    if (client == NULL) {
        return 1;
    }
    return client->_socket._closed;
}