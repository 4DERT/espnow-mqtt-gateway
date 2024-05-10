idf_component_register(SRCS
                       "main.c"
                       "esp-now-communication.c"
                       "ethernet_events.c"
                       "network_status.c"
                       "gateway_logic.c"
                       "gateway_device_list.c"
                       "mqtt.c"
                       "flash_filesystem.c"
                       "old_server_client.c" INCLUDE_DIRS ".")

spiffs_create_partition_image(storage ../partition FLASH_IN_PROJECT)