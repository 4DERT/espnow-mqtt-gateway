#include "settings.h"

#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "settings";

settings_t current_settings;

settings_param_t params[] = {
    {"addr", "MQTT Address", TYPE_STRING, true, false, MAX_SETTINGS_LENGTH,
     current_settings.mqtt_address_uri},
    {"user", "MQTT Username", TYPE_STRING, false, false, MAX_SETTINGS_LENGTH,
     current_settings.mqtt_username},
    {"pass", "MQTT Password", TYPE_STRING, false, true, MAX_SETTINGS_LENGTH,
     current_settings.mqtt_password},
    {"topic", "MQTT Topic", TYPE_STRING, true, false, MAX_SETTINGS_LENGTH,
     current_settings.mqtt_topic}};

size_t settings_get_params_count(void) {
  return sizeof(params) / sizeof(params[0]);
}

settings_t settings_get() { return current_settings; }

// saves current settings to flash
void settings_save_to_flash() {
  FILE *file = fopen(SETTINGS_FILE_PATH, "wb");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file in write binary mode");
    return;
  }

  if (fwrite(&current_settings, sizeof(settings_t), 1, file)) {
    ESP_LOGE(TAG, "Failed to write to file");
  }

  fclose(file);
}

// load settings from flash
void settings_load_from_flash() {
  FILE *file = fopen(SETTINGS_FILE_PATH, "wb");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file in read binary mode");
    return;
  }

  if (fread(&current_settings, sizeof(settings_t), 1, file)) {
    ESP_LOGE(TAG, "Failed to read from file");
  }

  fclose(file);
}