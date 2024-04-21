#include "ethernet_module.h"

#include "driver/gpio.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static ethernet_module_event_cb user_event_cb;
static ethernet_module_got_ip_cb user_got_ip_cb;

void ethernet_module_init(ethernet_module_event_cb event_cb, ethernet_module_got_ip_cb got_ip_cb) {
  // Register callbacks
  user_event_cb = event_cb;
  user_got_ip_cb = got_ip_cb;

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Create the netif "network interface" for this device
  esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t *eth_netif = esp_netif_new(&netif_config);

  // create the mac and phy configs
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();

  phy_config.phy_addr = CONFIG_ETHERNET_MODULE_ETH_PHY_ADDR;
  phy_config.reset_gpio_num = CONFIG_ETHERNET_MODULE_ETH_PHY_RST_GPIO;
  esp_rom_gpio_pad_select_gpio(CONFIG_ETHERNET_MODULE_ETH_PIN_PHY_POWER);
  gpio_set_direction(CONFIG_ETHERNET_MODULE_ETH_PIN_PHY_POWER, GPIO_MODE_OUTPUT);
  gpio_set_level(CONFIG_ETHERNET_MODULE_ETH_PIN_PHY_POWER, 1);
  vTaskDelay(pdMS_TO_TICKS(10));

  // cut example down for readability --- just taking the USE_INTERNAL_ETHERNET
#if CONFIG_ETHERNET_MODULE_USE_INTERNAL_ETHERNET
  esp32_emac_config.smi_mdc_gpio_num = CONFIG_ETHERNET_MODULE_ETH_MDC_GPIO;
  esp32_emac_config.smi_mdio_gpio_num = CONFIG_ETHERNET_MODULE_ETH_MDIO_GPIO;
  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
#if CONFIG_ETHERNET_MODULE_ETH_PHY_IP101
  esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
#elif CONFIG_ETHERNET_MODULE_ETH_PHY_RTL8201
  esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif CONFIG_ETHERNET_MODULE_ETH_PHY_LAN87XX
  esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
#elif CONFIG_ETHERNET_MODULE_ETH_PHY_DP83848
  esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
#endif
#endif  // CONFIG_ETHERNET_MODULE_USE_INTERNAL_ETHERNET

  // populate the ethernet config and install the driver
  esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
  esp_eth_handle_t eth_handle = NULL;
  ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

  // Go back set the default handlers for the ethernet network interface
  // ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, (esp_event_handler_t)user_event_cb, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, (esp_event_handler_t)user_got_ip_cb, NULL));

  /* attach Ethernet driver to the TCP/IP stack, this is more than a little cryptic */
  ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

  /* let packets flow! */
  ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}