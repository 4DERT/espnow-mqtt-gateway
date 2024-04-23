#include "esp-now-communication.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "esp now com";

static void on_esp_now_data_send(const uint8_t *mac_addr, esp_now_send_status_t status);
static void on_esp_now_data_receive(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);
static void print_recv_info(const esp_now_recv_info_t *esp_now_info);

void esp_now_communication_init() {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // init wifi module
  esp_netif_init();
  esp_event_loop_create_default();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Print MAC
  uint8_t mac[6];
  esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
  printf("+MAC_ ADDR: 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\n",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // esp now init
  if (esp_now_init() != ESP_OK) {
    ESP_LOGE(TAG, "esp_now_init ERROR");
  }

  // register esp now callbacks
  esp_now_register_send_cb(on_esp_now_data_send);
  esp_now_register_recv_cb(on_esp_now_data_receive);
}

/**
 * @brief Callback function of sending ESPNOW data.
 */
static void on_esp_now_data_send(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (!status) {
    ESP_LOGI(TAG, "Data sent with status %d (received)", status);
  } else {
    ESP_LOGW(TAG, "Data not sent - status %d (not received)", status);
  }
}

/**
 * @brief Callback function of receiving ESPNOW data.
 */
static void on_esp_now_data_receive(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
  static char buf[ESP_NOW_MAX_DATA_LEN];
  print_recv_info(esp_now_info);

  if (data_len) {
    memset(buf, 0, sizeof(buf));
    memcpy(buf, data, data_len);

    printf("data len: %d\n", data_len);
    printf("data: %s\n", buf);

  } else {
    ESP_LOGE(TAG, "Received data has incorrect size");
  }
}

static void print_recv_info(const esp_now_recv_info_t *esp_now_info) {
  char mac_src_buff[32];
  sprintf(mac_src_buff, "%02X:%02X:%02X:%02X:%02X:%02X",
          esp_now_info->src_addr[0], esp_now_info->src_addr[1], esp_now_info->src_addr[2],
          esp_now_info->src_addr[3], esp_now_info->src_addr[4], esp_now_info->src_addr[5]);

  char mac_dest_buff[32];
  sprintf(mac_dest_buff, "%02X:%02X:%02X:%02X:%02X:%02X",
          esp_now_info->des_addr[0], esp_now_info->des_addr[1], esp_now_info->des_addr[2],
          esp_now_info->des_addr[3], esp_now_info->des_addr[4], esp_now_info->des_addr[5]);

  wifi_pkt_rx_ctrl_t ctrl;
  memset(&ctrl, 0, sizeof(wifi_pkt_rx_ctrl_t));
  memcpy(&ctrl, esp_now_info->rx_ctrl, sizeof(wifi_pkt_rx_ctrl_t));

  printf("\n******\n");
  printf("mac_src: %s\n", mac_src_buff);
  printf("mac_des: %s\n", mac_dest_buff);
  printf("rssi: %d dBm\n", ctrl.rssi);
  printf("channel: %d\n", ctrl.channel);
  printf("secondary_channel: %d\n", ctrl.secondary_channel);
  printf("timestamp: %d microsecond\n", ctrl.timestamp);
  printf("ampdu_cnt: %d\n", ctrl.ampdu_cnt); /**< the number of subframes aggregated in AMPDU */
  printf("sig_len: %d\n", ctrl.sig_len);     /**< length of packet including Frame Check Sequence(FCS) */
  printf("sig_mode: %d\n", ctrl.sig_mode);   /**< Protocol of the reveived packet, 0: non HT(11bg) packet; 1: HT(11n) packet; 3: VHT(11ac) packet */
  printf("rx_state: %d\n", ctrl.rx_state);   /**< state of the packet. 0: no error; others: error numbers which are not public */
}