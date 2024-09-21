#ifndef GATEWAY_LOGIC_H_
#define GATEWAY_LOGIC_H_

#include "esp_now.h"
#include "esp-now-communication.h"
#include "esp_mac.h"

#define GW_MACSTR "%02x%02x%02x%02x%02x%02x"

#define GW_PAIR_HEADER      "SHPR:"
#define GW_ACCEPT_PAIR      "SHPR:PAIRED"
#define GW_TOPIC_SPEC       "!?:"
#define GW_TOPIC_SPEC_STATUS "!S:"
#define GW_TOPIC_SPEC_DATA  "!D:"
#define GW_STATUS_CHAR      'S'
#define GW_DATA_CHAR        'D'
#define GW_TOPIC_PREFIX     "device/"
#define GW_TOPIC_CMD        GW_TOPIC_PREFIX GW_MACSTR "/cmd"
#define GW_TOPIC_STATUS     GW_TOPIC_PREFIX GW_MACSTR "/status"
#define GW_TOPIC_AVAILABILITY     GW_TOPIC_PREFIX GW_MACSTR "/availability"
#define GW_TOPIC_DATA       GW_TOPIC_PREFIX GW_MACSTR "/data"
#define GW_AVAILABILITY_ONLINE    "online"
#define GW_AVAILABILITY_OFFLINE   "offline"
#define GW_GATEWAY_AVAILABILITY   "availability"
#define GW_GATEWAY_DEVICE_LIST_TOPIC "paired"
#define GW_PAIR_MAX_TIME_S 30
#define GW_GATEWAY_PENDING_PAIRING_TOPIC "pending_pairing"
#define GW_GATEWAY_ACCEPT_PAIR_TOPIC "accept_pair"
#define GW_USER_NAME_SIZE 16

typedef struct mac_t { uint8_t x[ESP_NOW_ETH_ALEN]; } mac_t;

typedef struct device_t {
    uint8_t mac[ESP_NOW_ETH_ALEN];
    char pair_msg[ESP_NOW_MAX_DATA_LEN];
    char user_name[GW_USER_NAME_SIZE];

    // private
    // A field indicating whether the slot in array is occupied
    bool _is_taken;
} device_t;

void gw_espnow_broadcast_parser(espnow_event_receive_cb_t* packet);
void gw_mqtt_parser(const char* topic, int topic_len, const char* data, int data_len);
void gw_espnow_message_parser(espnow_event_receive_cb_t* data);
void gw_subscribe_devices();
void gw_publish_paired_devices();
void gw_publish_pending_pairing_devices();
void gw_pair(mac_t* mac);
void gw_unpair(mac_t* mac);
void gw_rename(mac_t* mac, const char* name);

#endif //GATEWAY_LOGIC_H_