#include "gif_player.h"
#include "display.h"
#include "status_bar.h"
#include <esp_log.h>
#include <esp_spiffs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>

static const char *TAG = "gif_player";

#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240
#define FRAME_BUF_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * 2)
#define MAX_STATE_LEN  31

// Global state for coordinating with animation tasks
static char g_current_state[MAX_STATE_LEN + 1] = "";
static atomic_uint g_state_version = 0;
static bool g_spiffs_mounted = false;

// Forward declaration
static void anim_task(void *arg);

// ---------------------------------------------------------------------------
// Animation task: self-terminating task that plays frames for one state
// ---------------------------------------------------------------------------
static void anim_task(void *arg) {
    char *my_state = (char *)arg;

    // Read frame metadata
    int count = 1;
    int delay_ms = 100;
    char path[80];
    snprintf(path, sizeof(path), "/spiffs/%s/frames.txt", my_state);
    FILE *f = fopen(path, "r");
    if (f) {
        int n = fscanf(f, "%d:%d", &count, &delay_ms);
        fclose(f);
        if (n < 2) {
            // frames.txt exists but format is wrong; use defaults
            count = 1;
            delay_ms = 100;
        }
        ESP_LOGI(TAG, "state=%s frames=%d delay=%dms", my_state, count, delay_ms);
    } else {
        ESP_LOGW(TAG, "no frames.txt for state=%s, using defaults", my_state);
    }
    if (count < 1)     count = 1;
    if (delay_ms < 10) delay_ms = 100;

    // Allocate frame buffer in DMA-capable memory
    uint8_t *buf = (uint8_t *)heap_caps_malloc(FRAME_BUF_SIZE, MALLOC_CAP_DMA);
    if (!buf) {
        ESP_LOGE(TAG, "frame buffer alloc failed (%d bytes)", FRAME_BUF_SIZE);
        free(my_state);
        vTaskDelete(NULL);
        return;
    }

    int frame = 0;
    uint32_t my_version = g_state_version;
    int consecutive_failures = 0;
    while (1) {
        // Check if this task is still the active animation
        if (atomic_load(&g_state_version) != my_version) break;
        if (strcmp((const char *)g_current_state, my_state) != 0) {
            ESP_LOGD(TAG, "state changed, %s task exiting", my_state);
            break;
        }

        // Read frame file
        bool frame_read = false;
        snprintf(path, sizeof(path), "/spiffs/%s/%d.raw", my_state, frame);
        f = fopen(path, "r");
        if (f) {
            size_t nread = fread(buf, 1, FRAME_BUF_SIZE, f);
            fclose(f);
            if (nread == FRAME_BUF_SIZE) {
                status_bar_overlay((uint16_t *)buf, DISPLAY_WIDTH, DISPLAY_HEIGHT);
                display_draw_bitmap(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, buf);
                frame_read = true;
            } else {
                ESP_LOGW(TAG, "short read on %s (%d bytes)", path, nread);
            }
        } else {
            ESP_LOGW(TAG, "frame file not found: %s", path);
        }

        if (frame_read) {
            consecutive_failures = 0;
        } else {
            consecutive_failures++;
        }

        if (consecutive_failures >= count) {
            ESP_LOGW(TAG, "no valid frame files for state=%s, exiting", my_state);
            break;
        }

        frame = (frame + 1) % count;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    free(buf);
    free(my_state);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

esp_err_t gif_player_init(void) {
    ESP_LOGI(TAG, "mounting SPIFFS at /spiffs");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: total=%luKB used=%luKB",
                 (unsigned long)(total / 1024), (unsigned long)(used / 1024));
    }

    g_spiffs_mounted = true;
    ESP_LOGI(TAG, "GIF player ready");
    return ESP_OK;
}

esp_err_t gif_player_play(const char *state) {
    if (!g_spiffs_mounted) {
        ESP_LOGW(TAG, "SPIFFS not mounted, skipping playback");
        return ESP_ERR_INVALID_STATE;
    }
    if (!state || strlen(state) == 0) {
        ESP_LOGW(TAG, "empty state name");
        return ESP_ERR_INVALID_ARG;
    }

    // Update shared state — old animation tasks will notice and exit
    strlcpy(g_current_state, state, sizeof(g_current_state));
    g_state_version++;
    ESP_LOGI(TAG, "playing animation for state: %s", state);

    char *state_copy = strdup(state);
    if (!state_copy) return ESP_ERR_NO_MEM;

    TaskHandle_t task = NULL;
    BaseType_t res = xTaskCreate(anim_task, "gif_anim", 6144,
                                  state_copy, 5, &task);
    if (res != pdPASS) {
        free(state_copy);
        ESP_LOGE(TAG, "failed to create animation task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void gif_player_stop(void) {
    g_current_state[0] = '\0';
    display_clear(0x18E5);  // dark navy bg
    ESP_LOGI(TAG, "animation stopped");
}
