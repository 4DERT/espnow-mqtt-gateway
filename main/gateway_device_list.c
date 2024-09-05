#include "gateway_device_list.h"

#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "gateway_logic.h"
#include "mqtt.h"
#include "mqtt_client.h"

static const char* TAG = "gw_list";

static device_t device_list[GW_DEVICE_LIST_SIZE];
static int device_list_idx;

static void recalculate_idx();

const device_t* gw_get_device_list() {
  return device_list;
}

int gw_get_device_list_idx() {
  return device_list_idx;
}

device_t* gw_find_device_by_mac(uint8_t mac[]) {
  for (int i = 0; i <= device_list_idx; i++) {
    if (!memcmp(device_list[i].mac, mac, ESP_NOW_ETH_ALEN))
      return &device_list[i];
  }

  return NULL;
}

bool gw_add_device(device_t* device) {
  if (device_list_idx == GW_DEVICE_LIST_SIZE) {
    ESP_LOGW(TAG, "device list is full");
    return false;
  }

  if (gw_find_device_by_mac(device->mac) != NULL) {
    ESP_LOGW(TAG, "Device already paired");
    return false;
  }

  memcpy(&device_list[device_list_idx], device, sizeof(device_t));
  device_list_idx++;

  // save to flash
  gw_save_device_list_to_flash();

  return true;
}

void gw_save_device_list_to_flash() {
  FILE* file = fopen("/storage/device_list.bin", "wb");  // Otwarcie pliku do zapisu w trybie binarnym
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file in write binary mode");
    return;
  }

  if (fwrite(device_list, sizeof(device_t), GW_DEVICE_LIST_SIZE, file) != GW_DEVICE_LIST_SIZE) {
    ESP_LOGE(TAG, "Failed to write to file");
  }

  fclose(file);
}

void gw_load_device_list_from_flash() {
  FILE* file = fopen("/storage/device_list.bin", "rb");  // Otwarcie pliku do odczytu w trybie binarnym
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file in read binary mode");
    return;
  }

  if (fread(device_list, sizeof(device_t), GW_DEVICE_LIST_SIZE, file) != GW_DEVICE_LIST_SIZE) {
    ESP_LOGE(TAG, "Failed to read from file");
  }

  fclose(file);

  recalculate_idx();

  for (int i = 0; i < device_list_idx; i++) {
    device_list[device_list_idx].is_online = false;
  }
}

static void recalculate_idx() {
  int i;
  uint8_t empty_mac[ESP_NOW_ETH_ALEN] = {0};

  for (i = 0; i < GW_DEVICE_LIST_SIZE; i++) {
    if (memcmp(device_list[i].mac, empty_mac, ESP_NOW_ETH_ALEN) == 0) {
      device_list_idx = i;
      return;
    }
  }

  device_list_idx = GW_DEVICE_LIST_SIZE;
}
