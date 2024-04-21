idf_component_register(
    SRCS "ethernet_module.c" 
    INCLUDE_DIRS "."
    REQUIRES "esp_event" "driver" "esp_eth" "esp_netif")
