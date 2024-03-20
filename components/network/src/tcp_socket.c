#include <stdio.h>
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "common.h"

#include "tcp_socket.h"

const char *SOCKET_TAG = "tcps";

typedef struct tcp_t {
    int32_t _fd;
    int32_t _port;
    uint8_t _closed;
} tcp_t;

struct tcp_server_t 
{   tcp_t _tcp;
};

struct tcp_connection_t 
{   tcp_t _tcp;
    uint32_t _last_recv_time;
};

struct tcp_client_t 
{   tcp_t _tcp;
    uint32_t _last_recv_time;
};

/**
 * @brief Sends data over a tcp.
 *
 * This function sends the specified buffer of data over the tcp with the given file descriptor.
 * It sends the data in chunks until the entire buffer is sent.
 *
 * @param fd The file descriptor of the tcp.
 * @param buffer Pointer to the buffer containing the data to be sent.
 * @param buffer_len The length of the buffer in bytes.
 * @return The number of bytes sent on success, or -1 on failure.
 */
int64_t _tcp_send(int32_t fd, uint8_t *buffer, uint32_t buffer_len)
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
 * @brief Receives data from a tcp.
 *
 * This function receives data from the tcp with the given file descriptor and stores it in the specified buffer.
 * It returns the number of bytes received.
 *
 * @param fd The file descriptor of the tcp.
 * @param buffer Pointer to the buffer where the received data will be stored.
 * @param buffer_len The length of the buffer in bytes.
 * @return The number of bytes received on success, 0 if no data is available, -1 on failure, or -2 if the tcp has been disconnected.
 */
int64_t _tcp_recv(int32_t fd, uint8_t *buffer, uint32_t buffer_len)
{
    int64_t len = recv(fd, buffer, buffer_len, 0);
    if (len < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // Not an error
        }
        else if (errno == ENOTCONN) {
            ESP_LOGW(SOCKET_TAG, "Connection closed");
            return -2;  // tcp has been disconnected
        }
        ESP_LOGE(SOCKET_TAG, "Error occurred during receiving: errno %d", errno);
        return -1;
    }

    return len;
}

/**
 * @brief Closes a tcp.
 *
 * This function closes the tcp associated with the given tcp object.
 *
 * @param tcp Pointer to the tcp object.
 */
void _tcp_close(tcp_t *tcp)
{
    close(tcp->_fd);
    tcp->_closed = 1;
}

void _tcp_set_blocking(tcp_t *tcp, uint8_t blocking)
{
    int flags = (blocking) ? fcntl(tcp->_fd, F_GETFL) & ~O_NONBLOCK : fcntl(tcp->_fd, F_GETFL) | O_NONBLOCK;
    if (fcntl(tcp->_fd, F_SETFL, flags) == -1) {
        ESP_LOGE(SOCKET_TAG, "Unable to set tcp non blocking: errno %d", errno);
    }
}

tcp_server_t *tcp_server_create(uint32_t port, uint8_t blocking)
{
    tcp_server_t *server = (tcp_server_t *)malloc(sizeof(tcp_server_t));
    if (server == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to allocate memory for server");
        return NULL;
    }
    server->_tcp._port = port;
    server->_tcp._closed = 1;
    char port_str[6];
    snprintf(port_str, 6, "%lu", port);

    // Set the server address and port
    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;

    int res = getaddrinfo("0.0.0.0", port_str, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to resolve hostname for 0.0.0.0 getaddrinfo() returns %d, addrinfo=%p", res, address_info);
        tcp_server_free(server);
        return NULL;
    }

    // Create a tcp
    server->_tcp._fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (server->_tcp._fd < 0) {
        ESP_LOGE(SOCKET_TAG, "Unable to create tcp: errno %d", errno);
        tcp_server_free(server);
        return NULL;
    }
    server->_tcp._closed = 0;

    // Set the reuse address option
    int opt = 1;
    setsockopt(server->_tcp._fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server->_tcp._fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));

    // Set the tcp to non-blocking
    _tcp_set_blocking(&server->_tcp, blocking);

    ESP_LOGI(SOCKET_TAG, "tcp created");

    // Bind the tcp to the server
    int err = bind(server->_tcp._fd, address_info->ai_addr, address_info->ai_addrlen);
    if (err != 0) {
        ESP_LOGE(SOCKET_TAG, "tcp unable to bind: errno %d", errno);
        ESP_LOGE(SOCKET_TAG, "IPPROTO: %d", AF_INET);
        tcp_server_free(server);
        return NULL;
    }
    ESP_LOGI(SOCKET_TAG, "tcp bound, port %lu", port);
 
    // Start listening
    err = listen(server->_tcp._fd, 1);
    if (err != 0) {
        ESP_LOGE(SOCKET_TAG, "Error occurred during listen: errno %d", errno);
        tcp_server_free(server);
        return NULL;
    }
    return server;
}

tcp_connection_t *tcp_server_accept(tcp_server_t *server, uint8_t blocking)
{
    if (server == NULL) {
        return NULL;
    }
    
    tcp_connection_t *connection = (tcp_connection_t *)malloc(sizeof(tcp_connection_t));
    if (connection == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to allocate memory for connection");
        return NULL;
    }

    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);
    connection->_tcp._closed = 1;
    connection->_tcp._fd = accept(server->_tcp._fd, (struct sockaddr *)&source_addr, &addr_len);

    if (connection->_tcp._fd < 0) {
        if (errno == EWOULDBLOCK) 
        {
            ESP_LOGV(SOCKET_TAG, "No pending connections...");
            return NULL;
        }
        ESP_LOGE(SOCKET_TAG, "Unable to accept connection: errno %s", esp_err_to_name(errno));
        return NULL;
    }
    connection->_tcp._closed = 0;

    _tcp_set_blocking(&connection->_tcp, blocking);

    ESP_LOGI(SOCKET_TAG, "tcp accepted!");
    connection->_last_recv_time = get_time_ms();
    return connection;
}

void tcp_server_set_blocking(tcp_server_t *server, uint8_t blocking)
{
    if (server == NULL) {
        return;
    }
    _tcp_set_blocking(&server->_tcp, blocking);
}

void tcp_server_close(tcp_server_t *server)
{
    if (server == NULL) {
        return;
    }
    _tcp_close(&server->_tcp);
}

void tcp_server_free(tcp_server_t *server)
{
    if (server == NULL) {
        return;
    }

    if (server->_tcp._closed == 0) {
        tcp_server_close(server);
    }

    free(server);
}

void tcp_server_get_ip(tcp_server_t *server, char ip[INET6_ADDRSTRLEN])
{
    if (server == NULL) {
        return;
    }
    struct sockaddr_in6 addr;
    socklen_t addr_len = sizeof(addr);
    getsockname(server->_tcp._fd, (struct sockaddr *)&addr, &addr_len);
    inet_ntop(AF_INET6, &addr.sin6_addr, ip, INET6_ADDRSTRLEN);
}

uint32_t tcp_server_get_port(tcp_server_t *server)
{
    if (server == NULL) {
        return 0;
    }
    return server->_tcp._port;
}

uint8_t tcp_server_is_closed(tcp_server_t *server)
{
    if (server == NULL) {
        return 1;
    }
    return server->_tcp._closed;
}

void tcp_connection_set_blocking(tcp_connection_t *connection, uint8_t blocking)
{
    if (connection == NULL) {
        return;
    }
    _tcp_set_blocking(&connection->_tcp, blocking);
}

uint32_t tcp_connection_send(tcp_connection_t *connection, uint8_t *buffer, uint32_t buffer_len)
{
    if (connection == NULL) {
        return 0;
    }

    if (connection->_tcp._closed) {
        return 0;
    }

    if (get_time_ms() - connection->_last_recv_time > tcp_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        tcp_connection_close(connection);
        return 0;
    }

    int64_t bytes_sent = _tcp_send(connection->_tcp._fd, buffer, buffer_len);
    if (bytes_sent < 0) {
        tcp_connection_close(connection);
        return 0;
    }
    return (uint32_t)bytes_sent;
}

uint32_t tcp_connection_recv(tcp_connection_t *connection, uint8_t *buffer, uint32_t buffer_len)
{
    if (connection == NULL) {
        return 0;
    }

    if (connection->_tcp._closed) {
        return 0;
    }

    if (get_time_ms() - connection->_last_recv_time > tcp_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        tcp_connection_close(connection);
        return 0;
    }

    int64_t bytes_read = _tcp_recv(connection->_tcp._fd, buffer, buffer_len);
    if (bytes_read < 0) {
        tcp_connection_close(connection);
        return 0;
    }
    else if (bytes_read == 0) {
        return 0;
    }
    connection->_last_recv_time = get_time_ms();
    return (uint32_t)bytes_read;
}

void tcp_connection_close(tcp_connection_t *connection)
{
    if (connection == NULL) {
        return;
    }
    _tcp_close(&connection->_tcp);
}

uint8_t tcp_connection_is_closed(tcp_connection_t *connection)
{
    if (connection == NULL) {
        return 1;
    }
    return connection->_tcp._closed;
}

void tcp_connection_free(tcp_connection_t *connection)
{
    if (connection == NULL) {
        return;
    }

    if (connection->_tcp._closed == 0) {
        tcp_connection_close(connection);
    }

    free(connection);
}

tcp_client_t *tcp_client_create(const char *host_ip, uint32_t port, uint8_t blocking)
{
    tcp_client_t *client = (tcp_client_t *)malloc(sizeof(tcp_client_t));
    if (client == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to allocate memory for client");
        return NULL;
    }
    client->_tcp._port = port;
    client->_tcp._closed = 1;

    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;

    char port_str[6];
    snprintf(port_str, 6, "%lu", port);
    int res = getaddrinfo(host_ip, port_str, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(SOCKET_TAG, "Unable to resolve hostname for `%s` getaddrinfo() returns %d, addrinfo=%p", host_ip, res, address_info);
        goto error;
    }

    client->_tcp._fd = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (client->_tcp._fd < 0) {
        ESP_LOGE(SOCKET_TAG, "Unable to create tcp: errno %d", errno);
        goto error;
    }
    client->_tcp._closed = 0;

    _tcp_set_blocking(&client->_tcp, blocking);

    ESP_LOGI(SOCKET_TAG, "tcp created, connecting to %s:%lu", host_ip, port);

    // Connecting to the server
    if (connect(client->_tcp._fd, address_info->ai_addr, address_info->ai_addrlen)) {
        if (errno == EINPROGRESS) {
            struct pollfd pfd;
            pfd.fd = client->_tcp._fd;
            pfd.events = POLLOUT;

            res = poll(&pfd, 1, -1); // -1 means no timeout
            if (res < 0) {
                ESP_LOGE(SOCKET_TAG, "Error during connection: poll for tcp to be writable");
                goto error;
            } else if (res == 0) {
                ESP_LOGE(SOCKET_TAG, "Connection timeout: poll for tcp to be writable");
                goto error;
            } else {
                int sockerr;
                socklen_t len = (socklen_t)sizeof(int);
                if (getsockopt(client->_tcp._fd, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len) < 0) {
                    ESP_LOGE(SOCKET_TAG, "Error when getting tcp error using getsockopt()");
                    goto error;
                }
                if (sockerr) {
                    ESP_LOGE(SOCKET_TAG, "Connection error");
                    goto error;
                }
            }
        } else {
            ESP_LOGE(SOCKET_TAG, "tcp is unable to connect");
            goto error;
        }
    }
    ESP_LOGI(SOCKET_TAG, "Successfully connected");
    free(address_info);
    client->_last_recv_time = get_time_ms();
    return client;

    error:
        tcp_client_free(client);
        free(address_info);
        return NULL;
}

void tcp_client_set_blocking(tcp_client_t *client, uint8_t blocking)
{
    if (client == NULL) {
        return;
    }
    _tcp_set_blocking(&client->_tcp, blocking);
}

uint32_t tcp_client_send(tcp_client_t *client, uint8_t *buffer, uint32_t buffer_len)
{
    if (client == NULL) {
        return 0;
    }

    if (client->_tcp._closed) {
        return 0;
    }
    
    if (get_time_ms() - client->_last_recv_time > tcp_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        tcp_client_close(client);
        return 0;
    }
    
    int64_t bytes_sent = _tcp_send(client->_tcp._fd, buffer, buffer_len);
    if (bytes_sent < 0) {
        tcp_client_close(client);
        return 0;
    }
    return (uint32_t)bytes_sent;
}

uint32_t tcp_client_recv(tcp_client_t *client, uint8_t *buffer, uint32_t buffer_len)
{
    if (client == NULL) {
        return 0;
    }

    if (client->_tcp._closed) {
        return 0;
    }
    
    if (get_time_ms() - client->_last_recv_time > tcp_TIMEOUT_MS) {
        ESP_LOGW(SOCKET_TAG, "Connection timeout");
        tcp_client_close(client);
        return 0;
    }
    
    int64_t bytes_read = _tcp_recv(client->_tcp._fd, buffer, buffer_len);
    if (bytes_read < 0) {
        tcp_client_close(client);
        return 0;
    }
    else if (bytes_read == 0) {
        return 0;
    }
    client->_last_recv_time = get_time_ms();
    return (uint32_t)bytes_read;
}

void tcp_client_close(tcp_client_t *client)
{
    if (client == NULL) {
        return;
    }
    _tcp_close(&client->_tcp);
}

void tcp_client_free(tcp_client_t *client)
{
    if (client == NULL) {
        return;
    }

    if (client->_tcp._closed == 0) {
        tcp_client_close(client);
    }

    free(client);
}

uint8_t tcp_client_is_closed(tcp_client_t *client)
{
    if (client == NULL) {
        return 1;
    }
    return client->_tcp._closed;
}