#include "settings.h"



static const char* TAG = "settings";


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