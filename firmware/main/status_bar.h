#pragma once
#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t status_bar_init(void);
void status_bar_start_sntp(void);  // call after WiFi connected
// Render status bar directly into the top of a frame buffer.
void status_bar_overlay(uint16_t *buf, int width, int height);
void status_bar_update_time(void);

#ifdef __cplusplus
}
#endif
