#include <esp_log.h>
#include <mdns.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "wifi.h"
#include "websocket_client.h"
#include "state_machine.h"
#include "display.h"
#include "gif_player.h"
#include "status_bar.h"
#include "buttons.h"

static const char *TAG = "clawd";

// Bridge IP set by setup.sh (auto-detected from Mac's network interface)
#include "wifi_config.h"
#define BRIDGE_PORT 9120

static void on_state(const std::string &state) {
    auto s = clawd_state_from_string(state);
    ESP_LOGI(TAG, "state: %s", clawd_state_to_string(s));
    gif_player_play(state.c_str());
}

static void init_mdns() {
    mdns_init();
    mdns_hostname_set("clawd");
    mdns_instance_name_set("ESP32 Clawd");
    ESP_LOGI(TAG, "mDNS: clawd.local");
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "ESP32 Clawd starting...");

    // Initialize display (ST7789)
    esp_err_t ret = display_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "display initialized");
        display_clear(0x18E5);  // dark navy (Tokyo Night-inspired terminal bg)
    } else {
        ESP_LOGE(TAG, "display init failed: %s", esp_err_to_name(ret));
    }

    // Initialize status bar (time, wifi, battery)
    status_bar_init();

    // Initialize hardware buttons
    buttons_init();


    // Initialize GIF player (mount SPIFFS)
    ret = gif_player_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gif_player init failed: %s", esp_err_to_name(ret));
    }

    // Play default idle animation
    gif_player_play("idle");

    // WiFi
    wifi_init_sta();
    if (wifi_connected_sem) {
        xSemaphoreTake(wifi_connected_sem, pdMS_TO_TICKS(30000));
    }

    // Start SNTP after WiFi is up
    status_bar_start_sntp();

    // mDNS
    init_mdns();

    // WebSocket to bridge
    char url[128];
    snprintf(url, sizeof(url), "ws://%s:%d", BRIDGE_IP, BRIDGE_PORT);
    ws_client_start(url, on_state);

    ESP_LOGI(TAG, "ready");
    vTaskSuspend(NULL);
}
