idf_component_register(SRCS
                       "main.c"
                       "esp-now-communication.c"
                       "ethernet_events.c"
                       "network_status.c"
                       "gateway_logic.c"
                       "gateway_device_list.c"
                       "mqtt.c"
                       "flash_filesystem.c"
                       "webserver.c"
                       "settings_page.c"
                       "settings.c"
                       "device_info_collector.c"
                       "url_utils.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES
                       "web/favicon.ico"
                       "web/index.html"
                       "web/settings.html"
                       "web/styles.css")

spiffs_create_partition_image(storage ../partition FLASH_IN_PROJECT)