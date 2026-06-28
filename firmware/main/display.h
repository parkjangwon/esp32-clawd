#pragma once
#include <esp_err.h>
#include <stdint.h>

esp_err_t display_init(void);
esp_err_t display_draw_bitmap(int x_start, int y_start, int x_end, int y_end, const void *color_data);
void display_set_backlight(uint8_t percent);
void display_sleep(void);
void display_wake(void);
void display_clear(uint16_t color);
