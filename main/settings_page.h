#ifndef SETTINGS_PAGE_H_
#define SETTINGS_PAGE_H_

#include <esp_http_server.h>


extern httpd_uri_t settings_page_post;
extern httpd_uri_t settings_page_get;
extern httpd_uri_t settings_page_json_get;

#endif // SETTINGS_PAGE_H_