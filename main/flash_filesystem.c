#include "flash_filesystem.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"

static const char *TAG = "filesystem";

void flash_filesystem_init() {
  esp_vfs_spiffs_conf_t config = {
      .base_path = FLASH_BASE_PATH,
      .partition_label = NULL,
      .max_files = 2,
      .format_if_mount_failed = true};

  esp_err_t result = esp_vfs_spiffs_register(&config);

  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(result));
    return;
  }

  // get partition info
  size_t total = 0, used = 0;
  result = esp_spiffs_info(config.partition_label, &total, &used);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get partition info (%s)", esp_err_to_name(result));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
}