#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdbool.h>
#include <stddef.h>

#define MAX_SETTINGS_LENGTH 32
#define SETTINGS_FILE_PATH "/storage/settings.bin"

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
  char mqtt_address_uri[MAX_SETTINGS_LENGTH];
  char mqtt_username[MAX_SETTINGS_LENGTH];
  char mqtt_password[MAX_SETTINGS_LENGTH];
  char mqtt_topic[MAX_SETTINGS_LENGTH];
} settings_t;

extern settings_param_t params[];

extern size_t settings_get_params_count(void);
extern bool settings_save_to_flash();
extern bool settings_load_from_flash();

#endif // SETTINGS_H_