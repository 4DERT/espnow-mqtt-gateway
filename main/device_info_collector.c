#include "device_info_collector.h"

#include "cJSON.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

#include "esp-now-communication.h"
#include "gateway_device_list.h"

static const char *TAG = "DIC";

static dic_device_t device_list[DIC_DEVICE_LIST_SIZE];
static SemaphoreHandle_t device_list_mutex;

static QueueHandle_t dic_device_queue;

int find_slot() {
  for (int i = 0; i < DIC_DEVICE_LIST_SIZE; i++) {
    if (!device_list[i]._is_taken) {
      return i;
    }
  }
  return -1;
}

void analyze_data(dic_device_t *device) {
  if (xSemaphoreTake(device_list_mutex, portMAX_DELAY) == pdTRUE) {
    bool device_found = false;

    // find if mac exist in device_list
    for (int i = 0; i < DIC_DEVICE_LIST_SIZE; i++) {
      if (device_list[i]._is_taken &&
          memcmp(device_list[i].mac.x, device->mac.x, ESP_NOW_ETH_ALEN) == 0) {
        device_found = true;
        // update device in list
        memcpy(&device_list[i], device, sizeof(dic_device_t));
        device_list[i]._is_taken = true;
        break;
      }
    }

    // add new device to list
    if (!device_found) {
      int idx = find_slot();
      if (idx == -1) {
        ESP_LOGW(TAG, "Cannot find free space in device_list!");
      } else {
        memcpy(&device_list[idx], device, sizeof(dic_device_t));
        device_list[idx]._is_taken = true;
      }
    }

    xSemaphoreGive(device_list_mutex);
  }
}

void dic_update() {
    time_t current_time = time(NULL);

    if (xSemaphoreTake(device_list_mutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < DIC_DEVICE_LIST_SIZE; i++) {
            dic_device_t *device = &device_list[i];

            // Check if the device slot is taken
            if (device->_is_taken) {
                
                // Check if the device is currently not paired
                if (!device->is_paired) {
                    // Try to find the device by its MAC address
                    device_t *found_device = gw_find_device_by_mac(device->mac.x);
                    if (found_device != NULL) {
                        // Update the device status to paired
                        device->is_paired = true;
                        device->can_be_paired = false;
                        device->pair_msg = found_device->pair_msg;
                        device->user_name = found_device->user_name;
                        ESP_LOGI(TAG, "Device with MAC: " MACSTR " is now paired.", MAC2STR(device->mac.x));
                        continue;  // Skip the removal process, as the device is now paired
                    }

                    // Calculate the time since the last message was received
                    double time_diff = difftime(current_time, device->last_msg_time);

                    // If more than 1 hour (3600 seconds) has passed, remove the device
                    if (time_diff > 3600) {
                        ESP_LOGI(TAG, "Removing unpaired device with MAC: " MACSTR, MAC2STR(device->mac.x));

                        // Mark the device slot as free
                        device->_is_taken = false;

                        continue;
                    }

                    // Check if device can still be paired
                    device->can_be_paired = ((strncmp(device->last_msg, GW_PAIR_HEADER, strlen(GW_PAIR_HEADER)) == 0) 
                        && ((time(NULL) - device->last_msg_time) < GW_PAIR_MAX_TIME_S));
                } else {
                    // If the device is paired, check if it's still paired
                    device_t *found_device = gw_find_device_by_mac(device->mac.x);
                    if (found_device == NULL) {
                        // If the device is not found, update status to unpaired
                        device->is_paired = false;
                        device->can_be_paired = false;
                        device->pair_msg = NULL;
                        device->user_name = NULL;
                        ESP_LOGI(TAG, "Device with MAC: " MACSTR " has been unpaired.", MAC2STR(device->mac.x));
                    }
                }
            }
        }

        xSemaphoreGive(device_list_mutex);  // Release the mutex after the operation
    }
}



dic_device_t make_dic_device(espnow_event_receive_cb_t *data) {
  dic_device_t result = {0};
  result.rssi = data->esp_now_info.rx_ctrl->rssi;
  result.last_msg_time = time(NULL);

  memcpy(result.mac.x, data->esp_now_info.src_addr, ESP_NOW_ETH_ALEN);
  strcpy(result.last_msg, data->data);

  device_t *device = gw_find_device_by_mac(data->esp_now_info.src_addr);
  if(device != NULL) {
    result.is_paired = true;
    result.pair_msg = device->pair_msg;
    result.user_name = device->user_name;
  } else {
    // check if device can be paired
    result.can_be_paired = (strncmp(data->data, GW_PAIR_HEADER, strlen(GW_PAIR_HEADER)) == 0);
  }
  
  result._is_taken = false;

  return result;
}

void init_device_list() {
  const device_t *paired_devices = gw_get_device_list();

  for (int i = 0; i < GW_DEVICE_LIST_SIZE; i++) {
    if(!paired_devices[i]._is_taken) continue;
    
    // init device
    dic_device_t device = {0};
    device.is_paired = true;
    device._is_taken = true;
    memcpy(&device.mac, &paired_devices[i].mac, ESP_NOW_ETH_ALEN);
    device.pair_msg = paired_devices[i].pair_msg;
    device.user_name = paired_devices[i].user_name;

    // add to list
    int idx = find_slot();
    if (idx == -1) {
      ESP_LOGW(TAG, "Cannot find free space in device_list!");
    } else {
      memcpy(&device_list[idx], &device, sizeof(dic_device_t));
    }
  }
}

void dic_task(void *params) {
  BaseType_t queue_status;
  espnow_event_receive_cb_t data_raw;
  dic_device_t data;

  init_device_list();

  while (true) {
    // wait for new data (max 1s)
    queue_status =
        xQueueReceive(dic_device_queue, &data_raw, pdMS_TO_TICKS(1000));

    // analyze new data if exist
    if (queue_status == pdPASS) {
      data = make_dic_device(&data_raw);
      analyze_data(&data);
    }

    // frequently update devices in list
    // lock mutex and update device list (time, etc)
    dic_update();
  }
}

// Public

void dic_init() {
  dic_device_queue = xQueueCreate(DIC_DEVICE_QUEUE_SIZE, sizeof(dic_device_t));
  device_list_mutex = xSemaphoreCreateMutex();

  xTaskCreate(dic_task, "dic_task", 4096, NULL, 1, NULL);
}

void dic_log_device(const espnow_event_receive_cb_t *data) {
  if (xQueueSend(dic_device_queue, data, 0) != pdTRUE) {
    ESP_LOGE(TAG, "Error while adding device to DIC queue");
  }
}

void dic_get_device_list(dic_device_t **out_array,
                         SemaphoreHandle_t *out_mutex) {
  if (device_list_mutex != NULL) {
    *out_array = device_list;
    *out_mutex = device_list_mutex;
  } else {
    ESP_LOGE(TAG, "Device list mutex not initialized");
    *out_array = NULL;
    *out_mutex = NULL;
  }
}

void dic_print_device_list() {
  if (xSemaphoreTake(device_list_mutex, portMAX_DELAY) == pdTRUE) {
    printf(
        "Idx |    MAC Address    | Paired | RSSI |      Time      | Message\n");
    printf(
        "----|-------------------|--------|------|----------------|--------\n");

    for (int i = 0; i < DIC_DEVICE_LIST_SIZE; i++) {
      if (device_list[i]._is_taken) {
        printf("%-3d | " MACSTR " | %-6d | %-4d | %-14llu | %s\n", i,
               MAC2STR(device_list[i].mac.x), device_list[i].is_paired,
               device_list[i].rssi,
               (unsigned long long)device_list[i].last_msg_time,
               device_list[i].last_msg);
      }
    }

    xSemaphoreGive(device_list_mutex);
  }
}

char *dic_create_device_list_json() {
  cJSON *root = cJSON_CreateArray();

  for (int i = 0; i < DIC_DEVICE_LIST_SIZE; i++) {
    if (device_list[i]._is_taken) {
      cJSON *device = cJSON_CreateObject();

      char mac_str[18];
      snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(device_list[i].mac.x));

      cJSON_AddStringToObject(device, "mac", mac_str);
      cJSON_AddBoolToObject(device, "is_paired", device_list[i].is_paired);
      cJSON_AddBoolToObject(device, "can_be_paired", device_list[i].can_be_paired);
      cJSON_AddNumberToObject(device, "rssi", device_list[i].rssi);
      cJSON_AddNumberToObject(device, "last_msg_time",
                              (double)device_list[i].last_msg_time);
      cJSON_AddStringToObject(device, "user_name", device_list[i].user_name);
      if (device_list[i].pair_msg != NULL) {
          cJSON_AddStringToObject(device, "pair_msg", device_list[i].pair_msg);
      } else {
          cJSON_AddStringToObject(device, "pair_msg", "");
      }
      cJSON_AddStringToObject(device, "last_msg", device_list[i].last_msg);

      cJSON_AddItemToArray(root, device);
    }
  }

  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root); 

  return json_str;
}