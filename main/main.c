#include <stdio.h>

#include "esp_log.h"
#include "ethernet_events.h"
#include "ethernet_module.h"
#include "network_status.h"
#include "esp_time.h"
#include "esp-now-communication.h"
#include "mqtt.h"
#include "flash_filesystem.h"
#include "gateway_device_list.h"
#include "webserver.h"
#include "settings.h"
#include "device_info_collector.h"

static const char *TAG = "main";

void on_network_connected() {
  ESP_LOGI(TAG, "Network connected! - launching network-related tasks...");

  esp_time_sntp_init(NULL, NULL);
  mqtt_connected_notify();
}

void on_network_disconnected() {
  ESP_LOGI(TAG, "Network disconnected! - deleting network-related tasks...");

  esp_time_sntp_deinit();
}

void app_main(void) {
  flash_filesystem_init();
  settings_load_from_flash();
  gw_load_device_list_from_flash();
  init_network_status(on_network_connected, on_network_disconnected);
  ethernet_module_init(ethernet_even_handler, ethernet_got_ip_handler);
  dic_init();
  esp_now_communication_init();
  mqtt_init();
  webserver_start();

  vTaskDelete(NULL);
}
