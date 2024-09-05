#include "gateway_logic.h"

#include "esp-now-communication.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "gateway_device_list.h"
#include "mqtt.h"
#include "string.h"

// If it is 0, the gateway does not reject messages from unpaired devices. 
// Use with caution, preferably only in a development environment.
#define PAIR_IS_REQUIRED 0

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
    memcpy(device.pair_msg, data->data, data->data_len);
    gw_on_pair_request(&device);
  }
}

void gw_espnow_status_parse(espnow_event_receive_cb_t* data) {
  char msg[ESP_NOW_MAX_DATA_LEN];

  // Check message fomrat
  if (sscanf(data->data, GW_TOPIC_SPEC_STATUS "%s", msg) != 1) {
    ESP_LOGW(TAG, "Invalid message format");
    return;
  }

  char* topic = NULL;
  asprintf(&topic, GW_TOPIC_STATUS, MAC2STR(data->esp_now_info.src_addr));
  mqtt_publish(topic, msg, 0, 0, 0);
  free(topic);
}

void gw_espnow_data_parse(espnow_event_receive_cb_t* data) {
  char msg[ESP_NOW_MAX_DATA_LEN];

  // Check message fomrat
  if (sscanf(data->data, GW_TOPIC_SPEC_DATA "%s", msg) != 1) {
    ESP_LOGW(TAG, "Invalid message format");
    return;
  }

  char* topic = NULL;
  asprintf(&topic, GW_TOPIC_DATA, MAC2STR(data->esp_now_info.src_addr));
  mqtt_publish(topic, msg, 0, 0, 0);
  free(topic);
}

void gw_espnow_parse_no_type(espnow_event_receive_cb_t* data) {
  char* topic = NULL;
  asprintf(&topic, GW_TOPIC_PREFIX GW_MACSTR, MAC2STR(data->esp_now_info.src_addr));
  mqtt_publish(topic, data->data, 0, 0, 0);
  free(topic);
}

void gw_espnow_message_parser(espnow_event_receive_cb_t* data) {
  ESP_LOGD(TAG, "gw_espnow_message_parser");

#if PAIR_IS_REQUIRED == 1
  // check if device is paired
  device_t* device = gw_find_device_by_mac(data->esp_now_info.src_addr);
  if (device == NULL) {
    ESP_LOGE(TAG, "Device (" MACSTR ") not paired", MAC2STR(data->esp_now_info.src_addr));
    return;
  }

  // Send online
  if (device->is_online == false) {
    device->is_online = true;
    char* topic = NULL;
    asprintf(&topic, GW_TOPIC_STATUS, MAC2STR(data->esp_now_info.src_addr));
    mqtt_publish(topic, "online", 0, 0, 0);
    free(topic);
  }
#endif

  // check if device is specifying a topic using !D: or !S:
  if(data->data[0] == GW_TOPIC_SPEC[0] && data->data[2] == GW_TOPIC_SPEC[2]) {
    char msg_type = data->data[1];
    switch (msg_type) {
    case GW_STATUS_CHAR:
      gw_espnow_status_parse(data);
      break;
    
    case GW_DATA_CHAR:
      gw_espnow_data_parse(data);
      break;

    default:
      gw_espnow_parse_no_type(data);
      break;
    }
    
  } else {
    gw_espnow_parse_no_type(data);
  }

}

bool get_mac_from_topic(const char* topic, uint8_t* ret_mac) {
  const char* topic_ptr = topic + strlen(mqtt_get_topic_prefix()) + strlen(GW_TOPIC_PREFIX);

  for (int i = 0; i < 6; i++) {
    if (sscanf(topic_ptr, "%2hhx", &ret_mac[i]) != 1) {
      ESP_LOGE(TAG, "Error while retrieving mac address");
      return false;
    }
    topic_ptr += 2;  // Jump to nex segment (two hex char)
  }

  return true;
}

void gw_mqtt_parser(const char* topic, int topic_len, const char* data, int data_len) {
  ESP_LOGD(TAG, "gw_mqtt_parser");

  uint8_t mac[ESP_NOW_ETH_ALEN];
  if (!get_mac_from_topic(topic, mac)) {
    return;
  }

  device_t* device = gw_find_device_by_mac(mac);
  if (device == NULL) {
    ESP_LOGE(TAG, "Device with MAC " MACSTR " not found", MAC2STR(mac));
    return;
  }

  QueueHandle_t ack = esp_now_create_send_ack_queue();

  char* msg = NULL;
  asprintf(&msg, "%.*s", data_len, data);
  gw_send_to(device, msg, &ack);
  free(msg);

  esp_now_send_status_t res;
  xQueueReceive(ack, &res, portMAX_DELAY);
  ESP_LOGI(TAG, "data sent - status: %d", res);

  if (res == ESP_NOW_SEND_FAIL) {
    device->is_online = false;
    char* topic = NULL;
    asprintf(&topic, GW_TOPIC_STATUS, MAC2STR(mac));
    mqtt_publish(topic, "offline", 0, 0, 0);
    free(topic);
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

void gw_subscribe_devices() {
  int num_of_devices = gw_get_device_list_idx();
  if (num_of_devices == 0)
    return;

  esp_mqtt_topic_t* topic_list = calloc(num_of_devices, sizeof(esp_mqtt_topic_t));
  if (topic_list == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory\n");
    return;
  }

  const device_t* device_list = gw_get_device_list();

  const char* topic_prefix = mqtt_get_topic_prefix();
  for (int i = 0; i < num_of_devices; i++) {
    int needed_size = snprintf(NULL, 0, "%s" GW_TOPIC_CMD, topic_prefix, MAC2STR(device_list[i].mac)) + 1;  // +1 for null terminator
    char* dynamic_topic = malloc(needed_size);
    if (dynamic_topic == NULL) {
      ESP_LOGE(TAG, "Failed to allocate memory for topic string\n");
      continue;
    }
    snprintf(dynamic_topic, needed_size, "%s" GW_TOPIC_CMD, topic_prefix, MAC2STR(device_list[i].mac));
    topic_list[i].filter = dynamic_topic;  // Assign dynamically allocated topic
    topic_list[i].qos = 0;
  }

  // subscribe topics
  mqtt_subscribe_multiple_no_prefix(topic_list, num_of_devices);

  // send "online"
  for (int i = 0; i < num_of_devices; i++) {
    device_t* device = gw_find_device_by_mac(device_list[i].mac);
    device->is_online = true;
    char* topic = NULL;
    asprintf(&topic, GW_TOPIC_AVAILABILITY, MAC2STR(device_list[i].mac));
    mqtt_publish(topic, GW_AVAILABILITY_ONLINE, 0, 0, 1);
    free(topic);
  }

  // Free memory
  for (int i = 0; i < num_of_devices; i++) {
    free((void*)topic_list[i].filter);  // Free each dynamically allocated topic
  }
  free(topic_list);
}