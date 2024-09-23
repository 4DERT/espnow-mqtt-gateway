#ifndef ESP_NOW_COMMUNICATION_H_
#define ESP_NOW_COMMUNICATION_H_

#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

#define ESP_NOW_USE_CUSTOM_MAC CONFIG_ESP_NOW_USE_CUSTOM_MAC
#define ESP_NOW_MAC_ADDR CONFIG_ESP_NOW_MAC_ADDR
#define ESP_NOW_CHANNEL CONFIG_ESP_NOW_CHANNEL
#define ESP_NOW_SEND_QUEUE_SIZE CONFIG_ESP_NOW_SEND_QUEUE_SIZE
#define ESP_NOW_RECIEVE_QUEUE_SIZE CONFIG_ESP_NOW_RECIEVE_QUEUE_SIZE
#define ESP_NOW_RESULT_QUEUE_SIZE CONFIG_ESP_NOW_RESULT_QUEUE_SIZE

/*
Usage:

esp_now_communication_init();

QueueHandle_t result = esp_now_create_send_ack_queue();

esp_now_send_t data = {
    .data = "hello",
    .dest_mac = {0xC8, 0xC9, 0xA3, 0x30, 0x4E, 0x2E},
    .ack_queue = &result
};

xQueueSend(esp_now_send_queue, &data, 100);

esp_now_send_status_t res;
xQueueReceive(result, &res, portMAX_DELAY);
ESP_LOGI(TAG, "data sent - status: %d", res);
*/

typedef struct {
  char data[ESP_NOW_MAX_DATA_LEN];
  uint8_t dest_mac[ESP_NOW_ETH_ALEN];
  QueueHandle_t* ack_queue;
} esp_now_send_t;

typedef struct {
  esp_now_recv_info_t esp_now_info;
  char data[ESP_NOW_MAX_DATA_LEN];
  int data_len;
} espnow_event_receive_cb_t;

extern QueueHandle_t esp_now_send_queue;

QueueHandle_t esp_now_create_send_ack_queue();
void esp_now_communication_init();

#endif  // ESP_NOW_COMMUNICATION_H_