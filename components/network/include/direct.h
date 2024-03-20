/**
 * @file direct.h
 * @brief Direct wireless communication interface that wraps ESP-NOW
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "common.h"

typedef struct direct_peer_t {
    uint8_t mac[6];
} direct_peer_t;

/**
 * @brief Initializes the direct host with the specified user and password.
 *
 * This function initializes the direct host with the provided user and password.
 *
 * @param user The user name for authentication.
 * @param password The password for authentication.
 * @return The status code indicating the success or failure of the initialization.
 */
uint8_t direct_host_init(const char *user, const char *password);

/**
 * @brief Deinitializes the direct host.
 *
 * This function is responsible for deinitializing the direct host.
 * It should be called when the direct host is no longer needed.
 */
void direct_host_deinit(void);

/**
 * @brief Initializes the direct client.
 *
 * This function initializes the direct client.
 *
 * @return The status code indicating the success or failure of the initialization.
 */
uint8_t direct_client_init();

/**
 * @brief Deinitializes the direct client.
 *
 * This function is responsible for deinitializing the direct client.
 * It should be called when the direct client is no longer needed.
 */
void direct_client_deinit(void);

/**
 * @brief Connects the direct client to a server using the provided user and password.
 *
 * This function establishes a connection between the direct client and a server using the specified user and password.
 * The connection attempt will timeout after the specified timeout period.
 *
 * @param user The username to use for authentication.
 * @param password The password to use for authentication.
 * @param timeout_ms The timeout period in milliseconds for the connection attempt.
 * @return The status code indicating the success or failure of the connection attempt.
 */
uint8_t direct_client_connect(const char *user, const char *password, int64_t timeout_ms);

/**
 * @brief Disconnects the direct client from a server.
 *
 * This function disconnects the direct client from a server.
 *
 * @param peer The peer to disconnect from.
 */
void direct_client_disconnect(direct_peer_t *peer);

/**
 * @brief Retrieves the MAC address of the device.
 *
 * This function retrieves the MAC address of the device.
 *
 * @param mac The buffer to store the MAC address.
 */
void direct_get_mac(uint8_t *mac);

/**
 * @brief Retrieves the list of peers connected to the direct client.
 *
 * This function retrieves the list of peers connected to the direct client.
 *
 * @param peers The buffer to store the list of peers.
 * @param num_peers The number of peers in the list.
 */
void direct_get_peers(direct_peer_t *peers, uint8_t *num_peers);

/**
 * @brief Sends data to a specific peer.
 *
 * This function sends data to a specific peer identified by their MAC address.
 *
 * @param mac The MAC address of the peer.
 * @param buffer The buffer containing the data to send.
 * @param buffer_len The length of the data buffer.
 * @return The status code indicating the success or failure of the send operation.
 */
uint8_t direct_send(const uint8_t *mac, uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Receives data from a peer.
 *
 * This function receives data from a peer and stores it in the provided buffer.
 *
 * @param mac The buffer to store the MAC address of the sender.
 * @param buffer The buffer to store the received data.
 * @param buffer_len The length of the buffer to store the received data.
 * @return The status code indicating the success or failure of the receive operation.
 */
uint8_t direct_receive(uint8_t *mac, uint8_t *buffer, uint32_t *buffer_len);