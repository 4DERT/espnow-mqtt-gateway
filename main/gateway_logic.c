#include "gateway_logic.h"

#include "esp-now-communication.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "gateway_device_list.h"
#include "device_info_collector.h"
#include "mqtt.h"
#include "string.h"
#include "cJSON.h"
#include "settings.h"

static const char* TAG = "gateway";

static void gw_send_to(const uint8_t* mac, const char* msg, QueueHandle_t* ack_queue);
static bool gw_get_pair_msg_from_last_msg(const uint8_t mac[], char* out);

void extract_type_and_cfg_from_pair_msg(const char *msg, cJSON *json) {
    const char *prefix = GW_PAIR_HEADER; 
    size_t prefix_len = strlen(prefix);

    if (strncmp(msg, prefix, prefix_len) == 0) {
        // remove prefix
        const char *json_str = msg + prefix_len;

        cJSON *pairing_json = cJSON_Parse(json_str);
        if (pairing_json != NULL) {
            // Extract the "type" field from the JSON
            cJSON *type = cJSON_GetObjectItem(pairing_json, "type");
            if (type != NULL && cJSON_IsNumber(type)) {
                cJSON_AddNumberToObject(json, "type", type->valueint);
            }

            // Extract the "cfg" field from the JSON
            cJSON *cfg = cJSON_GetObjectItem(pairing_json, "cfg");
            if (cfg != NULL) {
                cJSON_AddItemToObject(json, "cfg", cJSON_Duplicate(cfg, 1));
            }

            // Free the parsed JSON memory
            cJSON_Delete(pairing_json);
        } else {
            ESP_LOGE(TAG, "Failed to parse pairing message: %s", json_str);
        }
    }
}


void gw_publish_pending_pairing_devices() {
    dic_device_t *device_list;
    SemaphoreHandle_t device_list_mutex;

    dic_get_device_list(&device_list, &device_list_mutex);

    if (device_list != NULL && device_list_mutex != NULL) {

        cJSON *root = cJSON_CreateArray();

        // Lock mutex for secure access to device list
        if (xSemaphoreTake(device_list_mutex, portMAX_DELAY) == pdTRUE) {
            
            for (int i = 0; i < DIC_DEVICE_LIST_SIZE; i++) {
                dic_device_t* device = &device_list[i];

                if (device->can_be_paired ) {
                    // Create a JSON object for the device
                    cJSON *cj_device = cJSON_CreateObject();

                    char mac_str[18];
                    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(device->mac.x));

                    cJSON_AddStringToObject(cj_device, "mac", mac_str);

                    extract_type_and_cfg_from_pair_msg(device->last_msg, cj_device);

                    // Add the device to the JSON array
                    cJSON_AddItemToArray(root, cj_device);
                }
            }

            // Free mutex
            xSemaphoreGive(device_list_mutex);

            char *json_str = cJSON_PrintUnformatted(root);
            cJSON_Delete(root); 

            mqtt_publish(GW_GATEWAY_PENDING_PAIRING_TOPIC, json_str, 0, 0, 1);

            free(json_str);

        } else {
            ESP_LOGE(TAG, "Failed to take device list mutex");
        }
    } else {
        ESP_LOGE(TAG, "Failed to get device list or mutex");
    }
}


void gw_on_pair_request(device_t* device) {
  ESP_LOGI(TAG, "Pair request from: " MACSTR, MAC2STR(device->mac));

  device_t* device_on_paired_list = gw_find_device_by_mac(device->mac);
  if (device_on_paired_list != NULL) {
    ESP_LOGW(TAG, "Device already paired");

    // update pair message
    char* new_pair_msg = malloc(ESP_NOW_MAX_DATA_LEN);
    if(new_pair_msg == NULL) {
      ESP_LOGE(TAG, "Memory allocation for new_pair_msg failed");
      return;
    }

    if(gw_get_pair_msg_from_last_msg(device->mac, new_pair_msg)) {
      gw_update_pair_message(device->mac, new_pair_msg);
    }
    free(new_pair_msg);

    // send accept pair message
    gw_send_to(device->mac, GW_ACCEPT_PAIR, NULL);
    return;
  }

  gw_publish_pending_pairing_devices();
  gw_publish_paired_devices();
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

  bool is_pair_not_required = settings_get().is_pair_not_required;

  if(!is_pair_not_required) {
    // check if device is paired
    device_t* device = gw_find_device_by_mac(data->esp_now_info.src_addr);
    if (device == NULL) {
      ESP_LOGE(TAG, "Device (" MACSTR ") not paired", MAC2STR(data->esp_now_info.src_addr));
      return;
    }
  }

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

bool gw_is_pair_accept_topic(const char* topic) {
  char *pair_accept_topic = NULL;
  asprintf(&pair_accept_topic, "%s%s", mqtt_get_topic_prefix(), GW_GATEWAY_ACCEPT_PAIR_TOPIC);

  if (pair_accept_topic == NULL) {
    ESP_LOGE(TAG, "Memory allocation for pair_accept_topic failed");
    return false;
  }

  if (strncmp(topic, pair_accept_topic, strlen(pair_accept_topic)) != 0) {
    free(pair_accept_topic); 
    return false;  // Topic does not match
  }

  free(pair_accept_topic);
  return true;
}

bool gw_get_pair_msg_from_last_msg(const uint8_t mac[], char* out) {
  dic_device_t *device_list;
  SemaphoreHandle_t device_list_mutex;
  dic_get_device_list(&device_list, &device_list_mutex);

  if (device_list != NULL && device_list_mutex != NULL) {

    // Lock mutex for secure access to device list
    if (xSemaphoreTake(device_list_mutex, portMAX_DELAY) == pdTRUE) {
      
      for (int i = 0; i < DIC_DEVICE_LIST_SIZE; i++) {
          dic_device_t* device = &device_list[i];

          if (device->_is_taken && 
              (strncmp(device->last_msg, GW_PAIR_HEADER, strlen(GW_PAIR_HEADER)) == 0) &&
              memcmp(device->mac.x, mac, ESP_NOW_ETH_ALEN) == 0) {

                snprintf(out, ESP_NOW_MAX_DATA_LEN, "%s", device->last_msg);

                // free mutex
                xSemaphoreGive(device_list_mutex);
                return true;

          }
      }

      // free mutex
      xSemaphoreGive(device_list_mutex);
      ESP_LOGE(TAG, "Failed to find device");

    } else {
      ESP_LOGE(TAG, "Failed to take device list mutex");
    }
  } else {
    ESP_LOGE(TAG, "Failed to get device list or mutex");
  }

  return false;
}

void gw_pair(mac_t* mac) {
  char* topic = NULL;
  asprintf(&topic, GW_TOPIC_CMD, MAC2STR(mac->x));

  device_t device;
  memcpy(device.mac, mac->x, ESP_NOW_ETH_ALEN);

  if(gw_get_pair_msg_from_last_msg(mac->x, device.pair_msg)) {
    gw_add_device(&device);

    mqtt_subscribe(topic, 2);
    gw_send_to(device.mac, GW_ACCEPT_PAIR, NULL);

    dic_update();
    gw_publish_paired_devices();
    gw_publish_pending_pairing_devices();
  } else {
    ESP_LOGE(TAG, "Error while pairing device!");
  }

  free(topic);
}

void gw_unpair(mac_t* mac) {
  gw_remove_device(mac);

  dic_update();
  gw_publish_paired_devices();
  gw_publish_pending_pairing_devices();
}

void gw_rename(mac_t* mac, const char* name) {
  gw_rename_device(mac, name);

  dic_update();
}

void gw_mqtt_parser(const char* topic, int topic_len, const char* data, int data_len) {
  ESP_LOGD(TAG, "gw_mqtt_parser");

  // Check if it is a pairing request
  if(gw_is_pair_accept_topic(topic)) {
    mac_t mac;

    if(sscanf(data, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
               &mac.x[0], &mac.x[1], &mac.x[2], &mac.x[3], &mac.x[4], &mac.x[5]) == 6) {
      gw_pair(&mac);
    } else {
      ESP_LOGE(TAG, "Error while pairing device - wrong mac");
    }

    return;
  }

  uint8_t mac[ESP_NOW_ETH_ALEN];
  if (!get_mac_from_topic(topic, mac)) {
    return;
  }

  if(!settings_get().is_pair_not_required) {
    device_t* device = gw_find_device_by_mac(mac);
    if (device == NULL) {
      ESP_LOGE(TAG, "Device with MAC " MACSTR " not found", MAC2STR(mac));
      return;
    }
  }

  QueueHandle_t ack = esp_now_create_send_ack_queue();

  char* msg = NULL;
  asprintf(&msg, "%.*s", data_len, data);
  gw_send_to(mac, msg, &ack);
  free(msg);

  esp_now_send_status_t res;
  xQueueReceive(ack, &res, portMAX_DELAY);
  ESP_LOGI(TAG, "data sent - status: %d", res);

  if (res == ESP_NOW_SEND_FAIL) {
    char* topic = NULL;
    asprintf(&topic, GW_TOPIC_STATUS, MAC2STR(mac));
    mqtt_publish(topic, "offline", 0, 0, 0);
    free(topic);
  }
}

void gw_send_to(const uint8_t* mac, const char* msg, QueueHandle_t* ack_queue) {
  esp_now_send_t data;
  memcpy(data.dest_mac, mac, ESP_NOW_ETH_ALEN);
  sprintf(data.data, msg);
  if (ack_queue != NULL)
    data.ack_queue = ack_queue;
  else
    data.ack_queue = NULL;

  xQueueSend(esp_now_send_queue, &data, portMAX_DELAY);
}

void gw_subscribe_devices() {
  bool is_pair_not_required = settings_get().is_pair_not_required;

  if(is_pair_not_required) {
    mqtt_subscribe(GW_TOPIC_CMD_ALL, 0);
  }

  int num_of_devices = gw_get_num_of_paired_devices();
  if (num_of_devices == 0)
    return;

  const device_t* device_list = gw_get_device_list();

  // subscribe only paired devices
  if(!is_pair_not_required) {
    esp_mqtt_topic_t* topic_list = calloc(num_of_devices, sizeof(esp_mqtt_topic_t));
    int topic_list_idx = 0;
    if (topic_list == NULL) {
      ESP_LOGE(TAG, "Failed to allocate memory\n");
      return;
    }

    const char* topic_prefix = mqtt_get_topic_prefix();
    for (int i = 0; i < GW_DEVICE_LIST_SIZE; i++) {
      if(!device_list[i]._is_taken) continue;

      int needed_size = snprintf(NULL, 0, "%s" GW_TOPIC_CMD, topic_prefix, MAC2STR(device_list[i].mac)) + 1;  // +1 for null terminator
      char* dynamic_topic = malloc(needed_size);
      if (dynamic_topic == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for topic string\n");
        continue;
      }
      snprintf(dynamic_topic, needed_size, "%s" GW_TOPIC_CMD, topic_prefix, MAC2STR(device_list[i].mac));
      topic_list[topic_list_idx].filter = dynamic_topic;  // Assign dynamically allocated topic
      topic_list[topic_list_idx].qos = 0;
      ++topic_list_idx;
    }

    // subscribe topics
    mqtt_subscribe_multiple_no_prefix(topic_list, num_of_devices);

    // Free memory
    for (int i = 0; i < num_of_devices; i++) {
      free((void*)topic_list[i].filter);  // Free each dynamically allocated topic
    }
    free(topic_list);
  }

  // send "online" for subscribed devices
  for (int i = 0; i < GW_DEVICE_LIST_SIZE; i++) {
    if(!device_list[i]._is_taken) continue;

    char* topic = NULL;
    asprintf(&topic, GW_TOPIC_AVAILABILITY, MAC2STR(device_list[i].mac));
    mqtt_publish(topic, GW_AVAILABILITY_ONLINE, 0, 0, 1);
    free(topic);
  }
}

void gw_publish_paired_devices() {
  const device_t* devices = gw_get_device_list();

  cJSON *root = cJSON_CreateArray();
  for (int i = 0; i < GW_DEVICE_LIST_SIZE; i++) {
      if(!devices[i]._is_taken) continue;

      cJSON *device = cJSON_CreateObject();

      char mac_str[18];
      snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(devices[i].mac));

      cJSON_AddStringToObject(device, "mac", mac_str);
      extract_type_and_cfg_from_pair_msg(devices[i].pair_msg, device);

      cJSON_AddItemToArray(root, device);
  }

  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root); 

  mqtt_publish(GW_GATEWAY_DEVICE_LIST_TOPIC, json_str, 0, 0, 1);

  free(json_str);
}