#ifndef COMPONENTS_ETHERNET_H_
#define COMPONENTS_ETHERNET_H_

#include "esp_event.h"
#include "esp_eth.h"
#include "esp_netif.h"

typedef void (*ethernet_module_event_cb)(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);

typedef void (*ethernet_module_got_ip_cb)(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data);

void ethernet_module_init(ethernet_module_event_cb user_event_cb, ethernet_module_got_ip_cb user_got_ip_cb);

#endif  // COMPONENTS_ETHERNET_H_