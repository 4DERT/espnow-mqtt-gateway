#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "esp_log.h"
#include "esp_time.h"
#include "freertos/FreeRTOS.h"

#if CONFIG_IDF_TARGET_ESP8266
#include "../../build/include/sdkconfig.h"
#include "FreeRTOSConfig.h"
#include "lwip/apps/sntp.h"
#include "portmacro.h"
#include "sntp.h"
#else  // esp32
#include "esp_sntp.h"
#endif

static const char* TAG = "ESP TIME SNTP";

void esp_time_sntp_init(char* sntp_server, char* time_zone) {
#if CONFIG_IDF_TARGET_ESP8266
  sntp_stop();
#else
  esp_sntp_stop();
#endif

  ESP_LOGI(TAG, "Initializing SNTP");
#if CONFIG_IDF_TARGET_ESP8266
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
#else
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
#endif

  if (sntp_server) {
#if CONFIG_IDF_TARGET_ESP8266
    sntp_setservername(0, sntp_server);
#else
    esp_sntp_setservername(0, sntp_server);
#endif
  } else {
#if CONFIG_IDF_TARGET_ESP8266
    sntp_setservername(0, CONFIG_ESP_TIME_NTP_SERVER);
#else
    esp_sntp_setservername(0, CONFIG_ESP_TIME_NTP_SERVER);
#endif
  }

#if CONFIG_IDF_TARGET_ESP8266
  sntp_init();
#else
  esp_sntp_init();
#endif

  if (time_zone)
    setenv("TZ", time_zone, 1);
  else
    setenv("TZ", CONFIG_ESP_TIME_TIME_ZONE, 1);

  tzset();
}

void esp_time_sntp_deinit() {
#if CONFIG_IDF_TARGET_ESP8266
  sntp_stop();
#else
  esp_sntp_stop();
#endif
}
