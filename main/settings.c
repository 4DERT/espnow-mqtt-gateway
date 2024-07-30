#include "settings.h"

#include "esp_log.h"
#include <stdio.h>

// The number of the objects to be read/write from file
#define SETTINGS_FILE_COUNT 1

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
bool settings_save_to_flash() {
  FILE *file = fopen(SETTINGS_FILE_PATH, "wb");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file in write binary mode");
    return false;
  }

  size_t written =
      fwrite(&current_settings, sizeof(settings_t), SETTINGS_FILE_COUNT, file);

  fclose(file);
  if (written != SETTINGS_FILE_COUNT) {
    ESP_LOGE(TAG, "Failed to write to file");
    return false;
  }

  return true;
}

// load settings from flash
bool settings_load_from_flash() {
  FILE *file = fopen(SETTINGS_FILE_PATH, "rb");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file in read binary mode");
    return false;
  }

  size_t readed =
      fread(&current_settings, sizeof(settings_t), SETTINGS_FILE_COUNT, file);

  fclose(file);

  if (readed != SETTINGS_FILE_COUNT) {
    ESP_LOGE(TAG, "Failed to read from file");
    return false;
  }

  return true;
}