#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "sdkconfig.h"

#include "old_server_client.h"

static const char *TAG = "OLD_SERVER_CLIENT";

static const char *REQUEST_TEMPLATE = "POST " WEB_PATH
                                      " HTTP/1.0\r\n"
                                      "Host: " WEB_SERVER ":" WEB_PORT
                                      "\r\n"
                                      "Content-Type: application/json\r\n"
                                      "Content-Length: %d\r\n"
                                      "User-Agent: esp-idf/1.0 esp32\r\n"
                                      "\r\n"
                                      "%s\r\n";

QueueHandle_t xQueueClient;
TaskHandle_t cln;

void http_get_task(void *pvParameters) {
  const struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *res;
  struct in_addr *addr;
  int s, r;
  char recv_buf[64];

  // queue
  char queue_data[256];
  char request[512];
  BaseType_t xStatus;

  while (1) {
    // WAIT FOR DATA
    memset(queue_data, 0, sizeof(queue_data));
    xStatus = xQueueReceive(xQueueClient, &queue_data, portMAX_DELAY);
    if (xStatus != pdPASS)
      continue;

    // create request
    const int length = strlen(queue_data);
    const int n = snprintf(request, sizeof(request), REQUEST_TEMPLATE, length, queue_data);

    if (0 > n || n >= sizeof(request)) {
      ESP_LOGE(TAG, "Buffer too small for the request");
      continue;
    }

    ESP_LOGI(TAG, "\n%s", request);

    // get address
    int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

    if (err != 0 || res == NULL) {
      ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    /* Code to print the resolved IP.
       Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0) {
      ESP_LOGE(TAG, "... Failed to allocate socket.");
      freeaddrinfo(res);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    ESP_LOGI(TAG, "... allocated socket");

    if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
      ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
      close(s);
      freeaddrinfo(res);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      continue;
    }

    ESP_LOGI(TAG, "... connected");
    freeaddrinfo(res);

    if (write(s, request, strlen(request)) < 0) {
      ESP_LOGE(TAG, "... socket send failed");
      close(s);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      continue;
    }
    ESP_LOGI(TAG, "... socket send success");

    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 5;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                   sizeof(receiving_timeout)) < 0) {
      ESP_LOGE(TAG, "... failed to set socket receiving timeout");
      close(s);
      vTaskDelay(4000 / portTICK_PERIOD_MS);
      continue;
    }
    ESP_LOGI(TAG, "... set socket receiving timeout success");

    /* Read HTTP response */
    do {
      bzero(recv_buf, sizeof(recv_buf));
      r = read(s, recv_buf, sizeof(recv_buf) - 1);
      for (int i = 0; i < r; i++) {
        putchar(recv_buf[i]);
      }
    } while (r > 0);

    ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
    close(s);
  }
}

void start_old_server_client() {
  if (cln == NULL) {
    xQueueClient = xQueueCreate(3, sizeof(char[256]));
    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 1, &cln);
  }
}

void stop_old_server_client() {
  if (cln != NULL) {
    vTaskDelete(cln);
    vQueueDelete(xQueueClient);

    cln = NULL;
    xQueueClient = NULL;
  }
}

BaseType_t send_to_old_server(const char *data) {
  if (cln == NULL || xQueueClient == NULL) {
    ESP_LOGE(TAG, "send_to_old_server - task is not started!");
    return pdFAIL;
  }

  char qdata[256];
  strcpy(qdata, data);

  return xQueueSend(xQueueClient, &qdata, 0);
}