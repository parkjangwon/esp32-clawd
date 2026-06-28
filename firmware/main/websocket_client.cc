#include "websocket_client.h"
#include <esp_websocket_client.h>
#include <esp_log.h>
#include <cJSON.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_system.h>
#include <string.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char *TAG = "ws";
static esp_websocket_client_handle_t client = nullptr;
static StateCallback state_callback = nullptr;
static char url_buffer[256];
static std::atomic<bool> should_reconnect{true};
static SemaphoreHandle_t ws_mutex = nullptr;

static void ws_event_handler(void *arg, esp_event_base_t base,
                              int32_t id, void *data) {
    auto *ev = (esp_websocket_event_data_t *)data;

    // Snapshot shared state under mutex
    if (ws_mutex) xSemaphoreTake(ws_mutex, portMAX_DELAY);
    StateCallback cb = state_callback;
    bool reconnect = should_reconnect.load();
    if (ws_mutex) xSemaphoreGive(ws_mutex);

    switch (id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "connected to bridge");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "disconnected");
        if (reconnect) {
            ESP_LOGI(TAG, "reconnecting in 3s...");
            // Reconnection handled by polling in main loop
        }
        break;
    case WEBSOCKET_EVENT_DATA:
        if (ev->data_len > 0 && ev->op_code != 0x02) {  // not binary frame (handle text + continuation)
            cJSON *root = cJSON_ParseWithLength(ev->data_ptr, ev->data_len);
            if (root) {
                cJSON *state = cJSON_GetObjectItem(root, "state");
                if (state && state->valuestring && cb) {
                    cb(state->valuestring);
                    ESP_LOGI(TAG, "state: %s", state->valuestring);
                }
                // Check for commands
                cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
                if (cmd && cmd->valuestring) {
                    if (strcmp(cmd->valuestring, "wifi") == 0) {
                        cJSON *ssid_j = cJSON_GetObjectItem(root, "ssid");
                        cJSON *pass_j = cJSON_GetObjectItem(root, "pass");
                        if (ssid_j && ssid_j->valuestring && pass_j && pass_j->valuestring) {
                            ESP_LOGI(TAG, "WiFi config received: %s", ssid_j->valuestring);
                            nvs_handle_t h;
                            if (nvs_open("wifi", NVS_READWRITE, &h) == ESP_OK) {
                                nvs_set_str(h, "ssid", ssid_j->valuestring);
                                nvs_set_str(h, "pass", pass_j->valuestring);
                                nvs_commit(h);
                                nvs_close(h);
                                ESP_LOGI(TAG, "WiFi saved, rebooting...");
                                vTaskDelay(pdMS_TO_TICKS(500));
                                esp_restart();
                            }
                        }
                    } else if (strcmp(cmd->valuestring, "erase_wifi") == 0) {
                        ESP_LOGI(TAG, "Erasing WiFi NVS...");
                        nvs_handle_t h;
                        if (nvs_open("wifi", NVS_READWRITE, &h) == ESP_OK) {
                            nvs_erase_all(h);
                            nvs_commit(h);
                            nvs_close(h);
                            ESP_LOGI(TAG, "WiFi NVS erased, rebooting...");
                            vTaskDelay(pdMS_TO_TICKS(500));
                            esp_restart();
                        }
                    }
                }
                cJSON_Delete(root);
            } else {
                ESP_LOGE(TAG, "json parse failed (len=%d)", ev->data_len);
            }
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "error");
        break;
    }
}

esp_err_t ws_client_start(const char *url, StateCallback on_state) {
    if (ws_mutex == nullptr) {
        ws_mutex = xSemaphoreCreateMutex();
    }
    xSemaphoreTake(ws_mutex, portMAX_DELAY);

    state_callback = on_state;
    strlcpy(url_buffer, url, sizeof(url_buffer));
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

    xSemaphoreGive(ws_mutex);
    return ESP_OK;
}

void ws_client_stop() {
    should_reconnect = false;
    if (ws_mutex) xSemaphoreTake(ws_mutex, portMAX_DELAY);
    if (client) {
        esp_websocket_client_stop(client);
        esp_websocket_client_destroy(client);
        client = nullptr;
    }
    state_callback = nullptr;
    if (ws_mutex) xSemaphoreGive(ws_mutex);
}
