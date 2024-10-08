#include "ethernet_events.h"

#include "esp_log.h"
#include "ethernet_module.h"
#include "network_status.h"

static const char *TAG = "ethernet events";

void ethernet_even_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data) {
  uint8_t mac_addr[6] = {0};
  /* get the ethernet driver handle from event data */
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

  switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
      esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
      ESP_LOGI(TAG, "Ethernet Link Up");
      ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
               mac_addr[0], mac_addr[1], mac_addr[2],
               mac_addr[3], mac_addr[4], mac_addr[5]);
      // network_status_take();
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "Ethernet Link Down");
      network_status_take();
      break;
    case ETHERNET_EVENT_START:
      ESP_LOGI(TAG, "Ethernet Started");
      // network_status_take();
      break;
    case ETHERNET_EVENT_STOP:
      ESP_LOGI(TAG, "Ethernet Stopped");
      // network_status_take();
      break;
    default:
      break;
  }
}

void ethernet_got_ip_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;

  ESP_LOGI(TAG, "Ethernet Got IP Address");
  ESP_LOGI(TAG, "~~~~~~~~~~~");
  ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(TAG, "~~~~~~~~~~~");

  network_status_give();
}
