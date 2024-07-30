#ifndef SETTINGS_PAGE_H_
#define SETTINGS_PAGE_H_

#include <esp_http_server.h>


extern httpd_uri_t settings_post;
extern httpd_uri_t settings_get;
extern httpd_uri_t settings_json_get;

#endif // SETTINGS_PAGE_H_