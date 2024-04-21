#ifndef ETHERNET_EVENTS_H_
#define ETHERNET_EVENTS_H_

#include "ethernet_module.h"

extern void ethernet_even_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);

extern void ethernet_got_ip_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);

#endif  // ETHERNET_EVENTS_H_