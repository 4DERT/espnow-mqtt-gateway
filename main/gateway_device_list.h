#ifndef GATEWAY_DEVICE_LIST_H_
#define GATEWAY_DEVICE_LIST_H_

#include "gateway_logic.h"

#define GW_DEVICE_LIST_SIZE 32
#define GW_NOT_FOUND -1

bool gw_add_device(device_t* device);
device_t* gw_find_device_by_mac(uint8_t mac[]);

#endif //GATEWAY_DEVICE_LIST_H_