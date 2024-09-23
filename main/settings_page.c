#include "settings_page.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_random.h"
#include "url_utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"

static const char *TAG = "httpd_settings";

/* Handler to respond with the contents of /settings */
esp_err_t settings_get_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "settings_get_handler");
  extern const char settings_html_start[] asm("_binary_settings_html_start");
  extern const char settings_html_end[] asm("_binary_settings_html_end");

  const size_t settings_html_size = (settings_html_end - settings_html_start);

  httpd_resp_send(req, settings_html_start, settings_html_size);
  return ESP_OK;
}

#define POST_BUF_SIZE 512

/* Handler to process settings form submission */
esp_err_t settings_post_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "settings_post_handler");

  if (req->content_len >= POST_BUF_SIZE) {
    return ESP_FAIL;
  }

  char buf[POST_BUF_SIZE];
  int ret, remaining = req->content_len;

  while (remaining > 0) {
    if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        continue;
      }
      return ESP_FAIL;
    }
    remaining -= ret;
  }

  buf[req->content_len] = '\0';
  url_decode(buf);

  printf("\n%s\n", buf);

  char temp_buf[MAX_SETTINGS_LENGTH];

  for (int i = 0; i < settings_get_params_count(); i++) {
    if (httpd_query_key_value(buf, params[i].name, temp_buf,
                              sizeof(temp_buf)) == ESP_OK) {
      switch (params[i].type) {
      case TYPE_STRING:
        strncpy((char *)params[i].value, temp_buf, params[i].size);
        break;
      case TYPE_INT:
        *(int *)params[i].value = atoi(temp_buf);
        break;
      case TYPE_BOOL:
        *(bool *)params[i].value = (strcmp(temp_buf, "on") == 0);
        break;
      }
    } else if (params[i].type == TYPE_BOOL) {
      *(bool *)params[i].value = false;
    }
  }

  settings_save_to_flash();

  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_sendstr(req, "Redirecting to the main page...");

  vTaskDelay(pdMS_TO_TICKS(100));

  esp_restart();
  return ESP_OK;
}

#define MAX_JSON_SIZE 2048

esp_err_t settings_json_handler(httpd_req_t *req) {
  static char json_buffer[MAX_JSON_SIZE];
  size_t len = snprintf(json_buffer, sizeof(json_buffer), "{\"settings\":[");

  for (int i = 0; i < settings_get_params_count(); i++) {
    if (i > 0) {
      len += snprintf(json_buffer + len, sizeof(json_buffer) - len, ",");
    }
    switch (params[i].type) {
    case TYPE_STRING:
      len += snprintf(json_buffer + len, sizeof(json_buffer) - len,
                      "{\"name\":\"%s\",\"label\":\"%s\",\"type\":\"string\","
                      "\"value\":\"%s\",\"is_required\":%s,\"is_password\":%s}",
                      params[i].name, params[i].label, (char *)params[i].value,
                      params[i].is_required ? "true" : "false",
                      params[i].is_password ? "true" : "false");
      break;
    case TYPE_INT:
      len += snprintf(
          json_buffer + len, sizeof(json_buffer) - len,
          "{\"name\":\"%s\",\"label\":\"%s\",\"type\":\"int\",\"value\":%"
          "d,"
          "\"is_required\":%s,\"is_password\":%s}",
          params[i].name, params[i].label, *(int *)params[i].value,
          params[i].is_required ? "true" : "false",
          params[i].is_password ? "true" : "false");
      break;
    case TYPE_BOOL:
      len += snprintf(
          json_buffer + len, sizeof(json_buffer) - len,
          "{\"name\":\"%s\",\"label\":\"%s\",\"type\":\"bool\",\"value\":"
          "%s,"
          "\"is_required\":%s,\"is_password\":%s}",
          params[i].name, params[i].label,
          (*(bool *)params[i].value) ? "true" : "false",
          params[i].is_required ? "true" : "false",
          params[i].is_password ? "true" : "false");
      break;
    }
  }

  len += snprintf(json_buffer + len, sizeof(json_buffer) - len, "]}");

  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json_buffer, strlen(json_buffer));
  return ESP_OK;
}

/* PAGES */

httpd_uri_t settings_page_post = {.uri = "/settings",
                                  .method = HTTP_POST,
                                  .handler = settings_post_handler,
                                  .user_ctx = NULL};

httpd_uri_t settings_page_get = {.uri = "/settings",
                                 .method = HTTP_GET,
                                 .handler = settings_get_handler,
                                 .user_ctx = NULL};

httpd_uri_t settings_page_json_get = {.uri = "/api/settings",
                                      .method = HTTP_GET,
                                      .handler = settings_json_handler,
                                      .user_ctx = NULL};
