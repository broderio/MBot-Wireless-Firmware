#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "common.h"
#include "pairing.h"
#include "direct.h"

#define DIRECT_TAG "DIRECT"

#define DIRECT_MAX_PACKET_SIZE 250
#define DIRECT_MAX_PEERS 5
#define DIRECT_WIFI_CHANNEL 11

#define DIRECT_ACK 0x01
#define DIRECT_DISCONNECT 0x02

#define DIRECT_QUEUE_SIZE 64

#define DIRECT_CONNECT_BIT BIT0

typedef struct direct_packet_t
{
    uint8_t mac[6];
    uint8_t *data;
    uint8_t len;
} direct_packet_t;

typedef enum direct_state_t
{
    DIRECT_CLIENT,
    DIRECT_HOST,
    DIRECT_NONE
} direct_state_t;

typedef enum direct_message_type_t
{
    DIRECT_FIRST,
    DIRECT_MIDDLE,
    DIRECT_LAST,
    DIRECT_SINGLE
} direct_message_type_t;

static QueueHandle_t direct_queue = NULL;
static QueueHandle_t direct_send_status_queue = NULL;

static direct_state_t direct_state = DIRECT_NONE;

static uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t direct_mac[6] = {0};

static uint8_t is_connected = 0;
static EventGroupHandle_t connect_event_group = NULL;

static uint8_t host_num_connections = 0;

static char direct_username[9] = {0};
static char direct_password[16] = {0};

void _direct_send_ack(const uint8_t *mac)
{
    uint8_t ack = DIRECT_ACK;
    direct_send(mac, &ack, 1);
}

void _direct_send_disconnect(const uint8_t *mac)
{
    uint8_t disconnect = DIRECT_DISCONNECT;
    direct_send(mac, &disconnect, 1);
}

void _direct_add_peer(const uint8_t *mac)
{
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        ESP_LOGE(DIRECT_TAG, "Malloc peer information fail");
        return;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = DIRECT_WIFI_CHANNEL;
    peer->ifidx = ESP_IF_WIFI_STA;
    peer->encrypt = 0;
    memcpy(peer->peer_addr, mac, ESP_NOW_ETH_ALEN);
    esp_now_add_peer(peer);
    free(peer);
}

static void _direct_client_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    uint8_t *src_addr = recv_info->src_addr;
    uint8_t *des_addr = recv_info->des_addr;

    if (src_addr == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(DIRECT_TAG, "Client receive cb arg error");
        return;
    }

    if (memcmp(des_addr, broadcast_mac, 6) == 0)
    {
        ESP_LOGD(DIRECT_TAG, "Received broadcast message from " MACSTR, MAC2STR(src_addr));
        return;
    }

    if (!is_connected && !esp_now_is_peer_exist(src_addr))
    {
        ESP_LOGW(DIRECT_TAG, "Received message from unknown peer " MACSTR, MAC2STR(src_addr));
        if (len == 1 && data[0] == DIRECT_ACK)
        {
            _direct_add_peer(src_addr);
            is_connected = 1;
            xEventGroupSetBits(connect_event_group, DIRECT_CONNECT_BIT);
            _direct_send_ack(src_addr);
            return;
        }
    }

    direct_packet_t packet = {0};
    packet.data = (uint8_t *)malloc(len);
    if (packet.data == NULL)
    {
        ESP_LOGE(DIRECT_TAG, "Failed to allocate memory for packet");
        return;
    }
    packet.len = len;
    memcpy(packet.data, data, len);
    memcpy(packet.mac, src_addr, 6);
    BaseType_t err = xQueueSend(direct_queue, &packet, 10 / portTICK_PERIOD_MS);
    if (err != pdTRUE)
    {
        ESP_LOGE(DIRECT_TAG, "Failed to send packet to direct queue");
        free(packet.data);
    }
}

static void _direct_host_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    uint8_t *src_addr = recv_info->src_addr;
    uint8_t *des_addr = recv_info->des_addr;

    if (src_addr == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(DIRECT_TAG, "Host receive cb arg error");
        return;
    }

    // Check if it is a message from a new device
    uint8_t is_new_peer = 0;
    if (!esp_now_is_peer_exist(src_addr))
    {
        ESP_LOGW(DIRECT_TAG, "Received message from unknown peer " MACSTR, MAC2STR(src_addr));
        is_new_peer = 1;
    }

    // Check if this is a broadcast message
    if (memcmp(des_addr, broadcast_mac, 6) == 0)
    {
        ESP_LOGD(DIRECT_TAG, "Received broadcast message from " MACSTR, MAC2STR(src_addr));

        // If a new peer is broadcasting
        if (is_new_peer && host_num_connections < DIRECT_MAX_PEERS)
        {
            if (len == sizeof(pair_config_t))
            {
                pair_config_t *pair_cfg = (pair_config_t *)data;

                // If the message matches the host username and password
                if (strcmp(pair_cfg->ssid, direct_username) == 0 && strcmp(pair_cfg->password, direct_password) == 0)
                {
                    ESP_LOGI(DIRECT_TAG, "Received connection request from " MACSTR, MAC2STR(src_addr));
                    // Send ACK to new peer
                    _direct_send_ack(src_addr);
                    _direct_add_peer(src_addr);
                    host_num_connections++;
                }
                else
                {
                    ESP_LOGW(DIRECT_TAG, "Received invalid connection request from " MACSTR, MAC2STR(src_addr));
                }
            }
        }
        return;
    }

    // If the message is from a new peer that isn't broadcasted, ignore it
    if (is_new_peer)
    {
        ESP_LOGW(DIRECT_TAG, "Received message from unknown peer " MACSTR, MAC2STR(src_addr));
        return;
    }

    // If the message is a disconnect from a peer
    if (len == 1 && data[0] == DIRECT_DISCONNECT)
    {
        ESP_LOGI(DIRECT_TAG, "Received disconnect from " MACSTR, MAC2STR(src_addr));
        esp_now_del_peer(src_addr);
        return;
    }

    direct_packet_t packet = {0};
    packet.data = (uint8_t *)malloc(len);
    if (packet.data == NULL)
    {
        ESP_LOGE(DIRECT_TAG, "Failed to allocate memory for packet");
        return;
    }
    packet.len = len;
    memcpy(packet.data, data, len);
    memcpy(packet.mac, src_addr, 6);
    BaseType_t err = xQueueSend(direct_queue, &packet, 10 / portTICK_PERIOD_MS);
    if (err != pdTRUE)
    {
        ESP_LOGE(DIRECT_TAG, "Failed to send packet to direct queue");
        free(packet.data);
    }
}

static void _direct_send_cb(const uint8_t *mac, esp_now_send_status_t status)
{
    BaseType_t err = xQueueSend(direct_send_status_queue, &status, 10 / portTICK_PERIOD_MS);
    if (err != pdTRUE)
    {
        ESP_LOGE(DIRECT_TAG, "Failed to send send status to direct queue");
    }
}

uint8_t _direct_init(void)
{
    esp_netif_init();

    esp_event_loop_create_default();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    esp_wifi_set_mode(ESP_IF_WIFI_STA);

    esp_wifi_start();

    esp_wifi_set_channel(DIRECT_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

    esp_now_init();

    direct_queue = xQueueCreate(DIRECT_QUEUE_SIZE, sizeof(direct_packet_t));
    direct_send_status_queue = xQueueCreate(DIRECT_QUEUE_SIZE, sizeof(esp_now_send_status_t));

    esp_now_register_send_cb(_direct_send_cb);

    esp_wifi_get_mac(ESP_IF_WIFI_STA, direct_mac);

    return 0;
}

void _direct_deinit(void)
{
    esp_now_unregister_send_cb();
    esp_now_unregister_recv_cb();

    vQueueDelete(direct_queue);
    vQueueDelete(direct_send_status_queue);

    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_event_loop_delete_default();
    esp_netif_deinit();
}

uint8_t direct_host_init(const char *user, const char *password)
{
    if (direct_state != DIRECT_NONE)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication already initialized");
        return 1;
    }

    if (user == NULL || password == NULL)
    {
        ESP_LOGE(DIRECT_TAG, "Invalid username or password");
        return 1;
    }

    direct_state = DIRECT_HOST;
    strncpy(direct_username, user, 9);
    strncpy(direct_password, password, 16);

    uint8_t err = _direct_init();
    if (err)
    {
        ESP_LOGE(DIRECT_TAG, "Failed to initialize direct communication");
        return 1;
    }

    esp_now_register_recv_cb(_direct_host_recv_cb);

    return 0;
}

void direct_host_deinit(void)
{
    if (direct_state != DIRECT_HOST)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return;
    }
    _direct_deinit();
}

uint8_t direct_client_init()
{
    if (direct_state != DIRECT_NONE)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication already initialized");
        return 1;
    }

    direct_state = DIRECT_CLIENT;

    uint8_t err = _direct_init();
    if (err)
    {
        ESP_LOGE(DIRECT_TAG, "Failed to initialize direct communication");
        return 1;
    }

    esp_now_register_recv_cb(_direct_client_recv_cb);
    connect_event_group = xEventGroupCreate();

    return 0;
}

void direct_client_deinit(void)
{
    if (direct_state != DIRECT_CLIENT)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return;
    }
    _direct_deinit();
    vEventGroupDelete(connect_event_group);
}

uint8_t direct_client_connect(const char *user, const char *password, int64_t timeout_ms)
{
    if (direct_state != DIRECT_CLIENT)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return 1;
    }

    if (user == NULL || password == NULL)
    {
        ESP_LOGE(DIRECT_TAG, "Invalid username or password");
        return 1;
    }

    pair_config_t pair_cfg = {0};
    strncpy(pair_cfg.ssid, user, 9);
    strncpy(pair_cfg.password, password, 16);

    xEventGroupClearBits(connect_event_group, DIRECT_CONNECT_BIT);

    direct_send(broadcast_mac, (uint8_t *)&pair_cfg, sizeof(pair_cfg));

    TickType_t ticks_to_wait = (timeout_ms < 0) ? portMAX_DELAY : timeout_ms / portTICK_PERIOD_MS;
    EventBits_t bits = xEventGroupWaitBits(connect_event_group, DIRECT_CONNECT_BIT, pdTRUE, pdTRUE, ticks_to_wait);
    if (bits & DIRECT_CONNECT_BIT)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void direct_client_disconnect(direct_peer_t *peer)
{
    if (direct_state != DIRECT_CLIENT)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return;
    }
    _direct_send_disconnect(peer->mac);
    esp_now_del_peer(peer->mac);
}

void direct_get_mac(uint8_t *mac)
{
    if (direct_state == DIRECT_NONE)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return;
    }
    memcpy(mac, direct_mac, 6);
}

void direct_get_peers(direct_peer_t *peers, uint8_t *num_peers)
{
    if (direct_state == DIRECT_NONE)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return;
    }
    esp_now_peer_info_t tmp_peer;
    memset((void *)&tmp_peer, 0, sizeof(esp_now_peer_info_t));
    esp_err_t err;
    int i = 0, j = 0;

    esp_now_peer_num_t peer_num;
    esp_now_get_peer_num(&peer_num);

    if (peer_num.total_num > 0)
    {
        peers = (direct_peer_t *)calloc(peer_num.total_num, sizeof(direct_peer_t));

        while ((j < peer_num.total_num) && (i < ESP_NOW_MAX_TOTAL_PEER_NUM))
        {
            err = esp_now_fetch_peer((i == 0), &tmp_peer);
            if (err == ESP_OK)
            {
                memcpy(&peers[j], &tmp_peer.peer_addr, sizeof(direct_peer_t));
                j++;
            }
            i++;
        }
    }
    *num_peers = j;
}

uint8_t direct_send(const uint8_t *mac, uint8_t *buffer, uint32_t buffer_len)
{
    if (direct_state == DIRECT_NONE)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return 1;
    }
    uint8_t out_buffer[DIRECT_MAX_PACKET_SIZE];

    uint32_t bytes_left = buffer_len;
    uint32_t offset = 0;
    while (bytes_left > 0)
    {
        uint32_t len = (bytes_left > DIRECT_MAX_PACKET_SIZE - 1) ? DIRECT_MAX_PACKET_SIZE - 1 : bytes_left;

        uint8_t type;
        if (bytes_left == buffer_len)
        {
            if (bytes_left == len)
            {
                type = DIRECT_SINGLE;
            }
            else
            {
                type = DIRECT_FIRST;
            }
        }
        else if (bytes_left == len)
        {
            type = DIRECT_LAST;
        }
        else
        {
            type = DIRECT_MIDDLE;
        }

        out_buffer[0] = type;
        memcpy(out_buffer + 1, buffer + offset, len);
        esp_now_send(mac, out_buffer, len + 1);

        esp_now_send_status_t status;
        xQueueReceive(direct_send_status_queue, &status, 25 / portTICK_PERIOD_MS);
        if (status != ESP_NOW_SEND_SUCCESS)
        {
            ESP_LOGE(DIRECT_TAG, "Failed to send packet");
            return 1;
        }

        bytes_left -= len;
        offset += len;
    }
    return 0;
}

uint8_t direct_receive(uint8_t *mac, uint8_t *buffer, uint32_t *buffer_len)
{
    if (direct_state == DIRECT_NONE)
    {
        ESP_LOGE(DIRECT_TAG, "Direct communication not initialized");
        return 1;
    }

    direct_packet_t packet;
    BaseType_t err = xQueueReceive(direct_queue, &packet, 10 / portTICK_PERIOD_MS);
    if (err != pdTRUE)
    {
        return 1;
    }

    uint32_t offset = 0;
    uint8_t type = packet.data[0];
    uint32_t len = packet.len - 1;
    memcpy(buffer + offset, packet.data + 1, len);
    *buffer_len = len;
    memcpy(mac, packet.mac, 6);
    free(packet.data);

    if (type == DIRECT_SINGLE)
    {
        return 0;
    }
    else if (type != DIRECT_FIRST)
    {
        return 1;
    }

    while (type != DIRECT_LAST)
    {
        BaseType_t err = xQueueReceive(direct_queue, &packet, 10 / portTICK_PERIOD_MS);
        if (err != pdTRUE)
        {
            return 1;
        }

        type = packet.data[0];
        len = packet.len - 1;
        memcpy(buffer + offset, packet.data + 1, len);
        *buffer_len += len;
        offset += len;
        free(packet.data);

        if (type != DIRECT_MIDDLE && type != DIRECT_LAST)
        {
            return 1;
        }
    }

    return 0;
}
