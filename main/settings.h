#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdbool.h>
#include <stddef.h>
#include "flash_filesystem.h"
#include "sdkconfig.h"

#define SETTINGS_MAX_STR_LENGTH CONFIG_SETTINGS_MAX_STR_LENGTH
#define SETTINGS_FILE_PATH CONFIG_SETTINGS_FILE_PATH

typedef enum { TYPE_STRING, TYPE_INT, TYPE_BOOL } settings_param_type_t;

typedef struct {
  const char *name;
  const char *label;
  settings_param_type_t type;
  bool is_required;
  bool is_password;
  size_t size;
  void *value;
} settings_param_t;

typedef struct {
  char mqtt_address_uri[SETTINGS_MAX_STR_LENGTH];
  char mqtt_username[SETTINGS_MAX_STR_LENGTH];
  char mqtt_password[SETTINGS_MAX_STR_LENGTH];
  char mqtt_topic[SETTINGS_MAX_STR_LENGTH];
  bool is_pair_not_required;
} settings_t;

extern settings_param_t params[];

extern size_t settings_get_params_count(void);
extern settings_t settings_get();
extern bool settings_save_to_flash();
extern bool settings_load_from_flash();

#endif // SETTINGS_H_