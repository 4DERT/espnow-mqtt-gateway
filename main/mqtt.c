#include "mqtt.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "gateway_logic.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT";

static esp_mqtt_client_handle_t client;
static bool is_connected;
static SemaphoreHandle_t init_semaphore;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void log_error_if_nonzero(const char *message, int error_code);

void mqtt_init() {
  init_semaphore = xSemaphoreCreateBinary();

  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = MQTT_BROKER_URI,
      .outbox.limit = MQTT_OUTBOX_LIMIT};

  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

  xSemaphoreTake(init_semaphore, MQTT_WAIT_FOR_CONNECTION_MS / portTICK_PERIOD_MS);
  esp_mqtt_client_start(client);
}

void mqtt_connected_notify() {
  xSemaphoreGive(init_semaphore);
}

void mqtt_stop() {
  esp_mqtt_client_stop(client);
  is_connected = false;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;

  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      is_connected = true;
      break;

    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      is_connected = false;
      break;

    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      break;

    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      break;

    case MQTT_EVENT_PUBLISHED:
      ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      break;

    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "MQTT_EVENT_DATA");
      printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      printf("DATA=%.*s\r\n", event->data_len, event->data);
      gw_mqtt_parser(event->topic, event->topic_len, event->data, event->data_len);
      break;

    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
        ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
      }
      break;

    default:
      ESP_LOGI(TAG, "Other event id:%d", event->event_id);
      break;
  }
}

static void log_error_if_nonzero(const char *message, int error_code) {
  if (error_code != 0) {
    ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
  }
}

bool mqtt_is_connected() {
  return is_connected;
}

void mqtt_subscribe(char *topic, int qos) {
  if (!is_connected) {
    ESP_LOGW(TAG, "No connection, topic will be subscribed after connection to the Internet");
  }

  int res = esp_mqtt_client_subscribe(client, topic, qos);
  if (res == -1 || res == -2) {
    ESP_LOGE(TAG, "Error (%d) while subscribing %s", res, topic);
  }
}

void mqtt_publish(const char *topic, const char *data, int len, int qos, int retain) {
  if (!is_connected) {
    ESP_LOGW(TAG, "No connection, message will be delivered after connection to the Internet");
  }

  int res = esp_mqtt_client_publish(client, topic, data, len, qos, retain);
  if (res == -1 || res == -2) {
    ESP_LOGE(TAG, "Error (%d) while publishing %s%s", res, topic, data);
  }
}