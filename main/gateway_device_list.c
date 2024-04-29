#include "gateway_device_list.h"

#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "gateway_logic.h"

static const char* TAG = "gw_list";

static device_t device_list[GW_DEVICE_LIST_SIZE];
static int device_list_idx;

int gw_find_device_in_list(const device_t* device) {
  for (int i = 0; i <= device_list_idx; i++) {
    if (!memcmp(device_list[i].mac, device->mac, ESP_NOW_ETH_ALEN))
      return i;
  }
  return GW_NOT_FOUND;
}

bool gw_add_device(device_t* device) {
  if (device_list_idx == GW_DEVICE_LIST_SIZE) {
    ESP_LOGW(TAG, "device list is full");
    return false;
  }

  if (gw_find_device_in_list(device) != GW_NOT_FOUND) {
    ESP_LOGW(TAG, "Device already paired");
    return false;
  }

  memcpy(&device_list[device_list_idx], device, ESP_NOW_ETH_ALEN);
  device_list_idx++;

  return true;
}