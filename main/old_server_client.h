#ifndef OLD_SERVER_CLIENT_H_
#define OLD_SERVER_CLIENT_H_

#include "freertos/FreeRTOS.h"

/*
 * This code is responsible for sending data to the old version of the server
 * it should be removed after the upgrade.
 */

#define WEB_SERVER "192.168.1.48"
#define WEB_PORT "8080"
#define WEB_PATH "/add"

void start_old_server_client();
void stop_old_server_client();
BaseType_t send_to_old_server(const char *data);

#endif  // OLD_SERVER_CLIENT_H_