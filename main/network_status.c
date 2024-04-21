#include "network_status.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "net_stat";

static TaskHandle_t network_status_task_handle;
static network_connected_cb user_on_network_connected;
static network_disconnected_cb user_on_network_disconnected;

static void network_status_task(void* params) {
  uint32_t current_connections = 0;
  uint32_t prev_connections = 0;
  uint32_t notified_value;

  while (1) {
    if (xTaskNotifyWait(0x00, ULONG_MAX, &notified_value, portMAX_DELAY)) {
      if (notified_value == 1) {  // increment
        current_connections++;
        ESP_LOGI(TAG, "Adding new connection");

      } else if (notified_value == 0 && current_connections > 0) {  // decrement
        if(current_connections) current_connections--;
        ESP_LOGI(TAG, "Lost connection");
      }

      ESP_LOGI(TAG, "Current number of connections: %lu", current_connections);

      // Checking the state of connections
      if (current_connections == 1 && prev_connections == 0) {
        if (user_on_network_connected) user_on_network_connected();
      }

      if (current_connections == 0) {
        if (user_on_network_disconnected) user_on_network_disconnected();
      }

      prev_connections = current_connections;
    }
  }
}

void init_network_status(network_connected_cb on_network_connected, network_disconnected_cb on_network_disconnected) {
  user_on_network_connected = on_network_connected;
  user_on_network_disconnected = on_network_disconnected;

  xTaskCreate(network_status_task, "net_stat_task",
              2048, NULL, 5, &network_status_task_handle);
}

inline void network_status_give() {
  xTaskNotify(network_status_task_handle, 1, eSetValueWithoutOverwrite);
}

inline void network_status_take() {
  xTaskNotify(network_status_task_handle, 0, eSetValueWithoutOverwrite);
}