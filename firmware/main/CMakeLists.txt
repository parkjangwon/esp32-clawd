idf_component_register(
    SRCS main.cc wifi.cc websocket_client.cc state_machine.cc display.cc gif_player.cc status_bar.cc buttons.cc
    INCLUDE_DIRS .
    REQUIRES nvs_flash esp_wifi esp_event esp_netif
    REQUIRES mdns json spiffs
    REQUIRES esp_websocket_client esp_adc
    REQUIRES esp_lcd esp_driver_spi esp_driver_ledc
)
