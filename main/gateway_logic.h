#ifndef GATEWAY_LOGIC_H_
#define GATEWAY_LOGIC_H_

#include "esp_now.h"
#include "esp-now-communication.h"
#include "esp_mac.h"

#define GW_PAIR_HEADER      "SHPR"
#define GW_ACCEPT_PAIR      "SHPR:PAIRED"
#define GW_STATUS_CHAR      'S'
#define GW_TOPIC_PREFIX     "/device/"
#define GW_TOPIC_CMD        GW_TOPIC_PREFIX MACSTR "/cmd/"
#define GW_TOPIC_STATUS     GW_TOPIC_PREFIX MACSTR "/status/"
#define GW_TOPIC_DATA       GW_TOPIC_PREFIX MACSTR "/data/"

typedef struct mac_t { uint8_t x[ESP_NOW_ETH_ALEN]; } mac_t;

typedef struct device_t {
    uint8_t mac[ESP_NOW_ETH_ALEN];
} device_t;

void gw_espnow_broadcast_parser(espnow_event_receive_cb_t* packet);
void gw_mqtt_parser(const char* topic, int topic_len, const char* data, int data_len);
void gw_espnow_message_parser(espnow_event_receive_cb_t* data);

#endif //GATEWAY_LOGIC_H_