#ifndef GATEWAY_LOGIC_H_
#define GATEWAY_LOGIC_H_

#include "esp_now.h"
#include "esp-now-communication.h"

#define GW_PAIR_HEADER "SHPR"
#define GW_ACCEPT_PAIR "SHPR:PAIRED"

typedef struct mac_t { uint8_t x[ESP_NOW_ETH_ALEN]; } mac_t;

typedef struct device_t {
    uint8_t mac[ESP_NOW_ETH_ALEN];
} device_t;

void gw_espnow_broadcast_parser(espnow_event_receive_cb_t* packet);

#endif //GATEWAY_LOGIC_H_