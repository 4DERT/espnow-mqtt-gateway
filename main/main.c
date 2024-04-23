#include <stdio.h>

#include "esp_log.h"
#include "ethernet_events.h"
#include "ethernet_module.h"
#include "network_status.h"
#include "esp_time.h"

static const char *TAG = "main";

void on_network_connected() {
  ESP_LOGI(TAG, "Network connected! - launching network-related tasks...");

  esp_time_sntp_init(NULL, NULL);
}

void on_network_disconnected() {
  ESP_LOGI(TAG, "Network disconnected! - deleting network-related tasks...");

  esp_time_sntp_deinit();
}

void app_main(void) {
  init_network_status(on_network_connected, on_network_disconnected);
  ethernet_module_init(ethernet_even_handler, ethernet_got_ip_handler);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
