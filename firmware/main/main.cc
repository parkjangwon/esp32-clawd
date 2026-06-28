#include <esp_log.h>
#include <mdns.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "wifi.h"
#include "websocket_client.h"
#include "state_machine.h"

static const char *TAG = "clawd";

// Bridge connection - mDNS for auto-discovery, fallback to direct IP
#define BRIDGE_HOST "clawd.local"
#define BRIDGE_PORT 9120

static void on_state(const std::string &state) {
    auto s = clawd_state_from_string(state);
    ESP_LOGI(TAG, "state: %s", clawd_state_to_string(s));
    // TODO: animation playback in Task 4
}

static void init_mdns() {
    mdns_init();
    mdns_hostname_set("clawd");
    mdns_instance_name_set("ESP32 Clawd");
    ESP_LOGI(TAG, "mDNS: clawd.local");
}

extern "C" void app_main() {
    ESP_LOGI(TAG, "ESP32 Clawd starting...");

    // WiFi
    wifi_init_sta();

    // Wait for WiFi to connect
    vTaskDelay(pdMS_TO_TICKS(5000));

    // mDNS
    init_mdns();

    // WebSocket to bridge
    char url[128];
    snprintf(url, sizeof(url), "ws://%s:%d", BRIDGE_HOST, BRIDGE_PORT);
    ws_client_start(url, on_state);

    // Main loop
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
