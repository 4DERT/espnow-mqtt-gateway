#include "gateway_logic.h"

#include "esp-now-communication.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "string.h"

#include "gateway_device_list.h"

static const char* TAG = "gateway";

static void gw_send_to(const device_t* device, const char* msg, QueueHandle_t* ack_queue);

void gw_on_pair_request(device_t* device) {
  ESP_LOGI(TAG, "Pair request from: " MACSTR, MAC2STR(device->mac));

  gw_add_device(device);

  // temporarly all pair request will be accepted
  // send "accept pair" message
  gw_send_to(device, GW_ACCEPT_PAIR, NULL);
}

void gw_espnow_broadcast_parser(espnow_event_receive_cb_t* data) {
  ESP_LOGD(TAG, "gw_espnow_broadcast_parser");

  // create device object
  device_t device;
  memcpy(device.mac, data->esp_now_info.src_addr, ESP_NOW_ETH_ALEN);

  // Check if this is pair request
  if (!strncmp(data->data, GW_PAIR_HEADER, strlen(GW_PAIR_HEADER))) {
    gw_on_pair_request(&device);
  }
}

void gw_send_to(const device_t* device, const char* msg, QueueHandle_t* ack_queue) {
  esp_now_send_t data;
  memcpy(data.dest_mac, device->mac, ESP_NOW_ETH_ALEN);
  sprintf(data.data, msg);
  if (ack_queue != NULL)
    data.ack_queue = ack_queue;
  else
    data.ack_queue = NULL;

  xQueueSend(esp_now_send_queue, &data, portMAX_DELAY);
}
