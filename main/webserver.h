#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <esp_http_server.h>

extern httpd_handle_t webserver_start(void);

#endif  // WEBSERVER_H_