#ifndef DEVICE_INFO_COLLECTOR_H_
#define DEVICE_INFO_COLLECTOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "sdkconfig.h"
#include "gateway_logic.h"

// Should be grater then GW_DEVICE_LIST
#define DIC_DEVICE_LIST_SIZE CONFIG_DIC_DEVICE_LIST_SIZE
#define DIC_DEVICE_QUEUE_SIZE CONFIG_DIC_DEVICE_QUEUE_SIZE

typedef enum {
  DIC_AVAILABILITY_UNKNOWN,
  DIC_AVAILABILITY_OFFLINE,
  DIC_AVAILABILITY_ONLINE
} dic_availability_e;

typedef struct {
  mac_t mac;
  bool is_paired;
  int rssi;
  time_t last_msg_time;
  const char* user_name;
  char last_msg[ESP_NOW_MAX_DATA_LEN];
  const char* pair_msg;
  bool can_be_paired;
  dic_availability_e availabilty;

  // private

  // A field indicating whether the slot in array is occupied
  bool _is_taken;
} dic_device_t;

extern void dic_init();

extern void dic_log_device(const espnow_event_receive_cb_t *data);

extern void dic_get_device_list(dic_device_t **out_array,
                                SemaphoreHandle_t *out_mutex);

extern void dic_print_device_list();

extern char *dic_create_device_list_json();

extern void dic_update();

extern void dic_mark_as_online(mac_t* mac);

extern void dic_mark_as_offline(mac_t* mac);

extern dic_device_t dic_get_device(mac_t* mac, bool* is_found);

#endif // DEVICE_INFO_COLLECTOR_H_
