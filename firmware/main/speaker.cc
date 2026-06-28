#include "speaker.h"
#include <esp_log.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "speaker";
#define SPK_PIN GPIO_NUM_7  // PA (amplifier enable) pin

esp_err_t speaker_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .gpio_num = SPK_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch);

    ESP_LOGI(TAG, "speaker ready (PWM on GPIO7)");
    return ESP_OK;
}

void speaker_beep(int freq_hz, int duration_ms) {
    if (freq_hz < 50) freq_hz = 50;
    if (freq_hz > 10000) freq_hz = 10000;

    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 64);  // 25% duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void speaker_success(void) {
    speaker_beep(800, 100);
    vTaskDelay(pdMS_TO_TICKS(80));
    speaker_beep(1200, 150);
}

void speaker_error(void) {
    speaker_beep(200, 300);
    vTaskDelay(pdMS_TO_TICKS(100));
    speaker_beep(200, 300);
}

void speaker_notify(void) {
    speaker_beep(1000, 80);
}
