#ifndef GATEWAY_DEVICE_LIST_H_
#define GATEWAY_DEVICE_LIST_H_

#include "gateway_logic.h"

#define GW_DEVICE_LIST_SIZE 32
#define GW_NOT_FOUND -1

int gw_find_device_in_list(const device_t* device);
inline bool gw_add_device(device_t* device);

#endif //GATEWAY_DEVICE_LIST_H_