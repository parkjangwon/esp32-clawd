#include "websocket_client.h"
#include <esp_websocket_client.h>
#include <esp_log.h>
#include <cJSON.h>
#include <string.h>

static const char *TAG = "ws";
static esp_websocket_client_handle_t client = nullptr;
static StateCallback state_callback = nullptr;
static char url_buffer[256];
static bool should_reconnect = true;

static void ws_event_handler(void *arg, esp_event_base_t base,
                              int32_t id, void *data) {
    auto *ev = (esp_websocket_event_data_t *)data;
    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "connected to bridge");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "disconnected");
        if (should_reconnect) {
            ESP_LOGI(TAG, "reconnecting in 3s...");
            // Reconnection handled by polling in main loop
        }
        break;
    case WEBSOCKET_EVENT_DATA:
        if (ev->data_len > 0 && !ev->op_code) {  // text frame
            cJSON *root = cJSON_ParseWithLength(ev->data_ptr, ev->data_len);
            if (root) {
                cJSON *state = cJSON_GetObjectItem(root, "state");
                if (state && state->valuestring && state_callback) {
                    state_callback(state->valuestring);
                    ESP_LOGI(TAG, "state: %s", state->valuestring);
                }
                cJSON_Delete(root);
            }
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "error");
        break;
    }
}

esp_err_t ws_client_start(const char *url, StateCallback on_state) {
    state_callback = on_state;
    strncpy(url_buffer, url, sizeof(url_buffer) - 1);
    should_reconnect = true;

    esp_websocket_client_config_t cfg = {};
    cfg.uri = url_buffer;
    cfg.disable_auto_reconnect = false;  // Let ESP handle reconnect
    cfg.reconnect_timeout_ms = 3000;
    cfg.network_timeout_ms = 10000;

    if (client) {
        esp_websocket_client_destroy(client);
        client = nullptr;
    }
    client = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, ws_event_handler, nullptr);
    esp_websocket_client_start(client);
    return ESP_OK;
}

void ws_client_stop() {
    should_reconnect = false;
    if (client) {
        esp_websocket_client_stop(client);
        esp_websocket_client_destroy(client);
        client = nullptr;
    }
}
