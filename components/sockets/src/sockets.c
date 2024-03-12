#include <stdio.h>
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "sockets.h"

const char *SOCKET_TAG = "sockets";

int socket_server_init(int port)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int flags = fcntl(listen_sock, F_GETFL);
    if (fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        ESP_LOGE(SOCKET_TAG, "Unable to set socket non blocking: errno %d", errno);
        return -1;
    }

    ESP_LOGI(SOCKET_TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(SOCKET_TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(SOCKET_TAG, "IPPROTO: %d", AF_INET);
        return -1;
    }
    ESP_LOGI(SOCKET_TAG, "Socket bound, port %d", port);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(SOCKET_TAG, "Error occurred during listen: errno %d", errno);
        return -1;
    }
    return listen_sock;
}

connection_socket_t socket_server_accept(server_socket_t server)
{
    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int sock = accept(server, (struct sockaddr *)&source_addr, &addr_len);

    if (sock < 0) {
        if (errno == EWOULDBLOCK) 
        {
            ESP_LOGV(SOCKET_TAG, "No pending connections...");
            return -2;
        }
        ESP_LOGE(SOCKET_TAG, "Unable to accept connection: errno %s", esp_err_to_name(errno));
        close(sock);
        return -1;
    }
    int keepAlive = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));

    int flags = fcntl(sock, F_GETFL);
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        ESP_LOGE(SOCKET_TAG, "Unable to set socket non blocking: errno %d", errno);
        return -1;
    }

    ESP_LOGI(SOCKET_TAG, "Socket accepted!");
    return sock;
}

int socket_server_close(server_socket_t server)
{
    return close(server);
}

int socket_connection_send(connection_socket_t connection, const char *data, int data_len)
{
    int bytes_sent = 0;
    while (bytes_sent < data_len) {
        int bytes = send(connection, data + bytes_sent, data_len - bytes_sent, 0);
        if (bytes < 0) {
            ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            return -1;
        }
        bytes_sent += bytes;
    }
    return 0;
}

int socket_connection_recv(connection_socket_t connection, char *buffer, int buffer_len)
{
    int len = recv(connection, buffer, buffer_len, 0);
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

int socket_connection_close(connection_socket_t connection)
{
    return close(connection);
}

client_socket_t socket_client_init(const char *host_ip, int port)
{
    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;

    char port_str[6];
    sprintf(port_str, "%d", port);
    int res = getaddrinfo(host_ip, port_str, &hints, &address_info);
    if (res != 0 || address_info == NULL) {
        ESP_LOGE(SOCKET_TAG, "couldn't get hostname for `%s` "
                      "getaddrinfo() returns %d, addrinfo=%p", host_ip, res, address_info);
        return -1;
    }

    // Creating client's socket
    int sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (sock < 0) {
        ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }
    int flags = fcntl(sock, F_GETFL);
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        ESP_LOGE(SOCKET_TAG, "Unable to set socket non blocking: errno %d", errno);
        return -1;
    }

    ESP_LOGI(SOCKET_TAG, "Socket created, connecting to %s:%d", host_ip, port);

    // Connecting to the server
    if (connect(sock, address_info->ai_addr, address_info->ai_addrlen) != 0) {
        if (errno == EINPROGRESS) {
            ESP_LOGD(SOCKET_TAG, "Connection in progress");
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);

            // Connection in progress -> have to wait until the connecting socket is marked as writable, i.e. connection completes
            res = select(sock+1, NULL, &fdset, NULL, NULL);
            if (res < 0) {
                // log_socket_error(TAG, sock, errno, "Error during connection: select for socket to be writable");
                ESP_LOGE(SOCKET_TAG, "Error during connection: select for socket to be writable");
                goto error;
            } else if (res == 0) {
                // log_socket_error(TAG, sock, errno, "Connection timeout: select for socket to be writable");
                ESP_LOGE(SOCKET_TAG, "Connection timeout: select for socket to be writable");
                goto error;
            } else {
                int sockerr;
                socklen_t len = (socklen_t)sizeof(int);

                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&sockerr), &len) < 0) {
                    // log_socket_error(TAG, sock, errno, "Error when getting socket error using getsockopt()");
                    ESP_LOGE(SOCKET_TAG, "Error when getting socket error using getsockopt()");
                    goto error;
                }
                if (sockerr) {
                    // log_socket_error(TAG, sock, sockerr, "Connection error");
                    ESP_LOGE(SOCKET_TAG, "Connection error");
                    goto error;
                }
            }
        } else {
            // log_socket_error(TAG, sock, errno, "Socket is unable to connect");
            ESP_LOGE(SOCKET_TAG, "Socket is unable to connect");
            goto error;
        }
    }
    ESP_LOGI(SOCKET_TAG, "Successfully connected");
    return sock;

    error:
        if (sock != -1) {
            close(sock);
        }
        free(address_info);
        return -1;
}

int socket_client_send(client_socket_t client, const char *data, int data_len)
{
    int bytes_sent = 0;
    while (bytes_sent < data_len) {
        int bytes = send(client, data + bytes_sent, data_len - bytes_sent, 0);
         if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // Not an error
        }
        else if (errno == ENOTCONN) {
            ESP_LOGW(SOCKET_TAG, "Connection closed");
            return -2;  // Socket has been disconnected
        }
        else if (bytes < 0) {
            ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            return -1;
        }
        
        bytes_sent += bytes;
        // ESP_LOGI(SOCKET_TAG, "Bytes sent: %d", bytes_sent);
    }
    return 0;
}

int socket_client_recv(client_socket_t client, char *buffer, int buffer_len)
{
    int len = recv(client, buffer, buffer_len, 0);
    if (len < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;   // Not an error
        }
        if (errno == ENOTCONN) {
            ESP_LOGW(SOCKET_TAG, "Connection closed");
            return -2;  // Socket has been disconnected
        }
        ESP_LOGE(SOCKET_TAG, "Error occurred during receiving: errno %d", errno);
        return -1;
    }

    return len;
}

int socket_client_close(client_socket_t client)
{
    return close(client);
}
