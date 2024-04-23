#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "FreeRTOSConfig.h"
#include "esp_time.h"

#if CONFIG_IDF_TARGET_ESP8266
#include "../../build/include/sdkconfig.h"
#include "FreeRTOSConfig.h"
#include "lwip/apps/sntp.h"
#include "portmacro.h"
#include "sntp.h"
#else  // esp32
#include "esp_sntp.h"
#endif

void esp_time_set_system_time(int YY, int MM, int DD, int hh, int mm, int ss, int isDST) {
  struct tm tm;

  tm.tm_year = YY - 1900;  // Set date
  tm.tm_mon = MM - 1;
  tm.tm_mday = DD;
  tm.tm_hour = hh;  // Set time
  tm.tm_min = mm;
  tm.tm_sec = ss;
  tm.tm_isdst = isDST;  // 1 or 0	`1` when setting the time during SUMMER, `0` when setting the time during WINTER.

  time_t t = mktime(&tm);

  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);
}

time_t esp_time_get_alarm_time(int YY, int MM, int DD, int hh, int mm, int ss) {
  time_t anow;
  struct tm atm;

  time(&anow);
  localtime_r(&anow, &atm);

  struct tm tm;

  tm.tm_year = YY - 1900;  // Set date
  tm.tm_mon = MM - 1;
  tm.tm_mday = DD;
  tm.tm_hour = hh;  // Set time
  tm.tm_min = mm;
  tm.tm_sec = ss;
  tm.tm_isdst = atm.tm_isdst;  // Daylight saving or standard time obtained from the system.

  time_t t = mktime(&tm);

  struct timeval now = {.tv_sec = t};
  settimeofday(&now, NULL);

  anow = mktime(&tm);

  return anow;
}

bool esp_time_is_time_changed(void) {
  time_t now;
  static time_t last;

  time(&now);
  if (last == now)
    return false;

  last = now;

  return true;
}

char* esp_time_get_formatted_datetime(time_t now, char* buf_fmt) {
  char buf[128];

  struct tm timeinfo;

  localtime_r(&now, &timeinfo);

  if (timeinfo.tm_year < (2016 - 1900)) {
    buf_fmt[0] = 0;
    return buf_fmt;
  } else {
    if (!buf_fmt[0]) {  // If the formatting string is empty, enforce the default format, e.g., 12:34:07 14.05.2022.
      strftime(buf, sizeof(buf), "%X  %d.%m.%Y", localtime(&now));
    } else {  // if buf_fmt is given
      strftime(buf, sizeof(buf), buf_fmt, localtime(&now));
    }
  }

  buf_fmt[0] = 0;
  strcat(buf_fmt, buf);  // Write to the string formatting the date and time according to the format.

  return buf_fmt;
}

char* esp_time_get_formatted_system_datetime(char* buf_fmt) {
  time_t now;
  time(&now);

  return esp_time_get_formatted_datetime(now, buf_fmt);
}