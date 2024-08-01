#ifndef DEVICE_INFO_COLLECTOR_H_
#define DEVICE_INFO_COLLECTOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "gateway_logic.h"

// Should be grater then GW_DEVICE_LIST
#define DIC_DEVICE_LIST_SIZE 64
#define DIC_DEVICE_QUEUE_SIZE 3

typedef struct {
  mac_t mac;
  bool is_paired;
  int rssi;
  time_t last_msg_time;
  char user_name[16];
  char last_msg[ESP_NOW_MAX_DATA_LEN];

  // private

  // A field indicating whether the slot in array is occupied
  bool _is_taken;
} dic_device_t;

extern void dic_init();

extern void dic_log_device(const espnow_event_receive_cb_t *data);

extern void dic_get_device_list(dic_device_t **out_array,
                                SemaphoreHandle_t *out_mutex);

extern void dic_print_device_list();

#endif // DEVICE_INFO_COLLECTOR_H_
