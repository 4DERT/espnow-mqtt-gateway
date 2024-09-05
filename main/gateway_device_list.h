#ifndef GATEWAY_DEVICE_LIST_H_
#define GATEWAY_DEVICE_LIST_H_

#include "gateway_logic.h"

#define GW_DEVICE_LIST_SIZE 32
#define GW_NOT_FOUND -1

const device_t* gw_get_device_list();
int gw_get_device_list_idx();
bool gw_add_device(device_t* device);
device_t* gw_find_device_by_mac(const uint8_t mac[]);
void gw_update_pair_message(const uint8_t mac[], const char* new_pair_msg);

void gw_save_device_list_to_flash();
void gw_load_device_list_from_flash();

#endif //GATEWAY_DEVICE_LIST_H_