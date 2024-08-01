#include "device_info_collector.h"

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

void update() {
  if (xSemaphoreTake(device_list_mutex, portMAX_DELAY) == pdTRUE) {
    // iterate over the device_list
    // check time of last message
    // if time is grater then 1 hour and device is not paired delete it from
    // device_list

    xSemaphoreGive(device_list_mutex);
  }
}

dic_device_t make_dic_device(espnow_event_receive_cb_t *data) {
  dic_device_t result = {0};
  result.rssi = data->esp_now_info.rx_ctrl->rssi;
  result.last_msg_time = time(NULL);

  memcpy(result.mac.x, data->esp_now_info.src_addr, ESP_NOW_ETH_ALEN);
  strcpy(result.last_msg, data->data);

  device_t *device = gw_find_device_by_mac(data->esp_now_info.src_addr);
  result.is_paired = !(device == NULL);

  result._is_taken = false;

  return result;
}

void init_device_list() {
  const device_t *paired_devices = gw_get_device_list();

  for (int i = 0; i < gw_get_device_list_idx(); i++) {
    // init device
    dic_device_t device = {0};
    device.is_paired = true;
    device._is_taken = true;
    memcpy(&device.mac, &paired_devices[i].mac, ESP_NOW_ETH_ALEN);

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
    update();
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
