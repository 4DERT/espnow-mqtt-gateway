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

const device_t* gw_get_device_list() {
  return device_list;
}

int gw_get_num_of_paired_devices() {
  int cnt = 0;

  for(size_t i = 0; i< GW_DEVICE_LIST_SIZE; i++) {
    if(device_list[i]._is_taken) cnt++;
  }

  return cnt;
}

device_t* gw_find_device_by_mac(const uint8_t mac[]) {
  for (int i = 0; i < GW_DEVICE_LIST_SIZE; i++) {
    if(!device_list[i]._is_taken) continue;

    if (!memcmp(device_list[i].mac, mac, ESP_NOW_ETH_ALEN))
      return &device_list[i];
  }

  return NULL;
}

bool gw_add_device(device_t* device) {
  // find free slot
  int slot = -1;
  for(int i = 0; i<GW_DEVICE_LIST_SIZE; i++) {
    if(!device_list[i]._is_taken) slot = i;
  }

  if(slot == -1) {
    ESP_LOGW(TAG, "device list is full");
    return false;
  }

  if (gw_find_device_by_mac(device->mac) != NULL) {
    ESP_LOGW(TAG, "Device already paired");
    return false;
  }

  memcpy(&device_list[slot], device, sizeof(device_t));
  memset(device_list[slot].user_name, 0, GW_USER_NAME_SIZE);

  // save to flash
  gw_save_device_list_to_flash();

  return true;
}

bool gw_remove_device(mac_t* mac) {
  device_t* device = gw_find_device_by_mac(mac->x);

  if (device != NULL) {
    device->_is_taken = false;
    gw_save_device_list_to_flash();
    return true; 
  }

  return false;
}

bool gw_rename_device(mac_t* mac, const char* name) {
  device_t* device = gw_find_device_by_mac(mac->x);

  if (device != NULL) {
    strncpy(device->user_name, name, GW_USER_NAME_SIZE);
    gw_save_device_list_to_flash();
    return true; 
  }

  return false;
}

void gw_update_pair_message(const uint8_t mac[], const char* new_pair_msg) {
  device_t* device = gw_find_device_by_mac(mac);

  if(device != NULL) {
    strncpy(device->pair_msg, new_pair_msg, ESP_NOW_MAX_DATA_LEN);
    gw_save_device_list_to_flash();
  } else {
    ESP_LOGE(TAG, "Error while updating pair message");
  }

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
}
