#include "gateway_logic.h"

#include "esp-now-communication.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "gateway_device_list.h"
#include "mqtt.h"
#include "string.h"

static const char* TAG = "gateway";

static void gw_send_to(const device_t* device, const char* msg, QueueHandle_t* ack_queue);

void gw_on_pair_request(device_t* device) {
  ESP_LOGI(TAG, "Pair request from: " MACSTR, MAC2STR(device->mac));
  char* topic = NULL;
  asprintf(&topic, GW_TOPIC_CMD, MAC2STR(device->mac));

  gw_add_device(device);

  mqtt_subscribe(topic, 2);
  free(topic);

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

void gw_espnow_status_parse(espnow_event_receive_cb_t* data) {
  char status[4];
  int num = -1;

  // Check message fomrat
  if (sscanf(data->data, "S:%d,%3s", &num, status) != 2) {
    ESP_LOGW(TAG, "Invalid message format");
  }

  char* topic = NULL;
  asprintf(&topic, GW_TOPIC_STATUS "%d", MAC2STR(data->esp_now_info.src_addr), num);
  mqtt_publish(topic, status, 0, 0, 0);
  free(topic);
}

void gw_espnow_message_parser(espnow_event_receive_cb_t* data) {
  ESP_LOGD(TAG, "gw_espnow_message_parser");

  char msg_type = data->data[0];

  switch (msg_type) {
    case GW_STATUS_CHAR:
      gw_espnow_status_parse(data);
      break;

    default:
      break;
  }
}

bool get_mac_from_topic(const char* topic, uint8_t* ret_mac) {
  const char* topic_ptr = topic + strlen(GW_TOPIC_PREFIX);

  for (int i = 0; i < 6; i++) {
    if (sscanf(topic_ptr, "%2hhx", &ret_mac[i]) != 1) {
      ESP_LOGE(TAG, "Error while retrieving mac address");
      return false;
    }
    topic_ptr += 3;  // Jump to nex segment (two hex char and ':')
  }

  return true;
}

void gw_mqtt_parser(const char* topic, int topic_len, const char* data, int data_len) {
  ESP_LOGD(TAG, "gw_mqtt_parser");

  uint8_t mac[ESP_NOW_ETH_ALEN];
  if (!get_mac_from_topic(topic, mac)) {
    return;
  }

  // temporary
  // device_t* device = gw_find_device_by_mac(mac);
  // if (device == NULL) {
  //   ESP_LOGE(TAG, "Device with MAC " MACSTR " not found", MAC2STR(mac));
  //   return;
  // }

  device_t device;
  memcpy(device.mac, mac, ESP_NOW_ETH_ALEN);

  char* msg = NULL;
  asprintf(&msg, "%.*s", data_len, data);
  gw_send_to(&device, msg, NULL);
  free(msg);
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
