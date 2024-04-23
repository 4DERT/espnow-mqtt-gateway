#ifndef ESP_TIME_H_
#define ESP_TIME_H_

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* *** SNTP *** */

/*!
    @brief Initialization of SNTP with automatic time synchronization every hour.
    It is recommended to call it after connecting to the internet and obtaining an IP address.

   @param sntp_server sntp server address, if NULL the DEFAULT_NTP_SERVER will be used
   @param time_zone time zone string, if NULL the DEFAULT_TIME_ZONE will be used
*/
void esp_time_sntp_init(char* sntp_server, char* time_zone);

/*!
    @brief Disables SNTP.
    Should be executed upon losing the connection.
*/
extern void esp_time_sntp_deinit();

/* *** SYSTEM TIME *** */

/*!
    @brief Set the system time in RTOS with daylight saving time specification
    @param YY Year
    @param MM Month
    @param DD Day
    @param hh Hour
    @param mm Minute
    @param ss Second
    @param isDST 0 for standard (winter) time, 1 for daylight saving (summer) time.
*/
extern void esp_time_set_system_time(int YY, int MM, int DD, int hh, int mm, int ss, int isDST);

/**
 * The function `esp_time_get_formatted_system_datetime` returns the formatted system date and time in
 * the specified format.
 *
 * @param buf_fmt The `buf_fmt` parameter is a pointer to a character array that specifies the format
 * in which you want the date and time to be formatted. It is used to define the format of the output
 * string that represents the date and time. If is NULL, the format `%X %d.%m.%Y` will be used.
 *
 * @return buf_fmt - formatted system datetime string based on the current time.
 */
extern char* esp_time_get_formatted_system_datetime(char* buf_fmt);

/* *** TOOLS *** */

/**
 * The function `esp_time_get_formatted_datetime` provides the time or date based on the `now` argument
 * in the specified strftime format or a default format if `buf_fmt` is NULL.
 *
 * @param now The `now` parameter is a time_t variable representing the current time.
 * @param buf_fmt The `buf_fmt` parameter is a pointer to a character array that specifies the format
 * in which the date and time should be displayed. If `buf_fmt` is NULL, the function will use the
 * default format `%X %d.%m.%Y` to display the date and time.
 *
 * @return formatted date and time string based on the `now` argument and the specified format in the `buf_fmt` parameter.
 */
extern char* esp_time_get_formatted_datetime(time_t now, char* buf_fmt);

/**
 * The function `esp_time_is_time_changed` checks if the time has changed by 1 second compared to the
 * previous call and returns a boolean value accordingly.
 *
 * @return The function `esp_time_is_time_changed` returns a `bool` value, which is either `true` or
 * `false`. It returns `true` if the time has changed compared to the previous function call by
 * detecting a difference of 1 second, and it returns `false` if the time has not changed.
 */
extern bool esp_time_is_time_changed(void);

/**
 * The function `esp_time_get_alarm_time` sets and returns the alarm time in `time_t` format, adjusting
 * for the current time zone and daylight saving time.
 *
 * @param YY Year
 * @param MM Month
 * @param DD Day
 * @param hh Hour
 * @param mm Minute
 * @param ss Second
 *
 * @return The function `esp_time_get_alarm_time` is returning the alarm time in `time_t` format,
 * taking into account the current time zone.
 */
extern time_t esp_time_get_alarm_time(int YY, int MM, int DD, int hh, int mm, int ss);

#endif  // ESP_TIME_H_