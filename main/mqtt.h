#ifndef MQTT_H_
#define MQTT_H_

#include <stdbool.h>
#include "mqtt_client.h"
#include "sdkconfig.h"

#define MQTT_WAIT_FOR_CONNECTION_MS CONFIG_MQTT_WAIT_FOR_CONNECTION_MS
#define MQTT_OUTBOX_LIMIT CONFIG_MQTT_OUTBOX_LIMIT

void mqtt_init();
void mqtt_connected_notify();
void mqtt_stop();
bool mqtt_is_connected();
void mqtt_subscribe(char *topic, int qos);
void mqtt_subscribe_multiple_no_prefix(const esp_mqtt_topic_t *topic_list, int size);
void mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);
const char* mqtt_get_topic_prefix();

#endif  // MQTT_H_