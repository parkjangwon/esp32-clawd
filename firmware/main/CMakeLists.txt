idf_component_register(
    SRCS main.cc wifi.cc websocket_client.cc state_machine.cc
    INCLUDE_DIRS .
    REQUIRES nvs_flash esp_wifi esp_event esp_netif
    REQUIRES mdns json spiffs
    REQUIRES esp_websocket_client
)
