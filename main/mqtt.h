#ifndef MQTT_H_
#define MQTT_H_

#include <stdbool.h>

#define MQTT_BROKER_URI "mqtt://192.168.1.6:1883/"
#define MQTT_WAIT_FOR_CONNECTION_MS 10000
#define MQTT_OUTBOX_LIMIT 25

void mqtt_init();
void mqtt_connected_notify();
void mqtt_stop();
bool mqtt_is_connected();
void mqtt_subscribe(char *topic, int qos);
void mqtt_publish(const char *topic, const char *data, int len, int qos, int retain);

#endif  // MQTT_H_