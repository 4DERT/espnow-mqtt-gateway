#include "webserver.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_random.h"

#include "settings_page.h"
#include "device_info_collector.h"

static const char *TAG = "webserver";


/* Handler to respond with the contents of favicon.ico */
esp_err_t favicon_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "favicon_get_handler");
  extern const char favicon_ico_start[] asm("_binary_favicon_ico_start");
  extern const char favicon_ico_end[] asm("_binary_favicon_ico_end");
  const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);

  httpd_resp_set_type(req, "image/x-icon");
  httpd_resp_send(req, favicon_ico_start, favicon_ico_size);
  return ESP_OK;
}

static httpd_uri_t favicon = {.uri = "/favicon.ico",
                              .method = HTTP_GET,
                              .handler = favicon_get_handler,
                              .user_ctx = NULL};

/* Handler to respond with the contents of index.html */
esp_err_t index_html_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "index_html_get_handler");
  extern const char index_html_start[] asm("_binary_index_html_start");
  extern const char index_html_end[] asm("_binary_index_html_end");

  const size_t index_html_size = (index_html_end - index_html_start);

  httpd_resp_send(req, index_html_start, index_html_size);
  return ESP_OK;
}

static httpd_uri_t index_html = {.uri = "/",
                                 .method = HTTP_GET,
                                 .handler = index_html_get_handler,
                                 .user_ctx = NULL};

/* Handler to respond with the contents of index.html */
esp_err_t styles_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "styles_get_handler");
  extern const char styles_start[] asm("_binary_styles_css_start");
  extern const char styles_end[] asm("_binary_styles_css_end");

  const size_t styles_size = (styles_end - styles_start);

  httpd_resp_set_type(req, "text/css");
  httpd_resp_send(req, styles_start, styles_size);
  return ESP_OK;
}

static httpd_uri_t styles_uri= {.uri = "/styles.css",
                                 .method = HTTP_GET,
                                 .handler = styles_get_handler,
                                 .user_ctx = NULL};


/* Handler to respond with the contents of index.html */
esp_err_t device_list_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "device_list_get_handler");

  char *json_str = dic_create_device_list_json();
  if (json_str) {
      ESP_LOGD(TAG, "%s\n", json_str);
      
      httpd_resp_set_type(req, "application/json");
      httpd_resp_send(req, json_str, strlen(json_str));
      free(json_str);

      return ESP_OK;
  }


  return ESP_ERR_HTTPD_ALLOC_MEM;
}

static httpd_uri_t device_list_uri= {.uri = "/api/devices",
                                 .method = HTTP_GET,
                                 .handler = device_list_get_handler,
                                 .user_ctx = NULL};

httpd_handle_t webserver_start(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;
  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &favicon);
    httpd_register_uri_handler(server, &index_html);
    // httpd_register_uri_handler(server, &settings_html);
    httpd_register_uri_handler(server, &styles_uri);
    httpd_register_uri_handler(server, &settings_page_get);
    httpd_register_uri_handler(server, &settings_page_post);
    httpd_register_uri_handler(server, &settings_page_json_get);
    httpd_register_uri_handler(server, &device_list_uri);
    ESP_LOGI(TAG, "ESP32 Web Server started");
  } else {
    ESP_LOGE(TAG, "ESP32 Web Server not started - ERROR");
  }

  return server;
}