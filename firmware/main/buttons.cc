#include "buttons.h"
#include "display.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "buttons";

// Waveshare board buttons
#define BTN_POWER  GPIO_NUM_5   // center
#define BTN_UP     GPIO_NUM_4   // right (volume up)
#define BTN_DOWN   GPIO_NUM_0   // left (volume down)

#define LONG_PRESS_MS    800
#define V_LONG_PRESS_MS  3000
#define DEBOUNCE_MS      30

static bool g_display_on = true;
static uint8_t g_brightness = 50;  // percent

static bool read_btn(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;  // active low
}

static void handle_power_action(int level) {
    if (level == 2) {
        // Very long press: deep sleep (power off), wake on any button
        ESP_LOGI(TAG, "power very-long -> deep sleep");
        display_set_backlight(0);
        display_sleep();
        esp_sleep_enable_ext0_wakeup(BTN_POWER, 0);  // wake on power button press
        esp_deep_sleep_start();
    } else if (level == 1) {
        // Long press: restart
        ESP_LOGI(TAG, "power long -> restart");
        esp_restart();
    } else {
        // Short press: toggle display
        g_display_on = !g_display_on;
        ESP_LOGI(TAG, "power short -> display %s", g_display_on ? "on" : "off");
        if (g_display_on) {
            display_set_backlight(g_brightness);
            display_wake();
        } else {
            display_set_backlight(0);
            display_sleep();
        }
    }
}

static void handle_vol_up() {
    if (g_brightness < 100) g_brightness += 10;
    if (g_display_on) display_set_backlight(g_brightness);
    ESP_LOGI(TAG, "vol+ brightness=%d%%", g_brightness);
}

static void handle_vol_down() {
    if (g_brightness > 10) g_brightness -= 10;
    if (g_display_on) display_set_backlight(g_brightness);
    ESP_LOGI(TAG, "vol- brightness=%d%%", g_brightness);
}

static void button_task(void *arg) {
    TickType_t power_down=0, up_down=0, down_down=0;
    bool up_handled=false, down_handled=false;

    while(1) {
        TickType_t now=xTaskGetTickCount();

        // ── Power button: actions fire on RELEASE ──
        //     short(<0.8s)=display toggle, long(0.8-3s)=restart, very-long(>3s)=power-off
        if(read_btn(BTN_POWER)) {
            if(power_down==0) power_down=now;
        } else if(power_down!=0) {
            int held = pdTICKS_TO_MS(now-power_down);
            if(held>DEBOUNCE_MS && held<V_LONG_PRESS_MS && held>=LONG_PRESS_MS){
                handle_power_action(1); // restart
            } else if(held>=V_LONG_PRESS_MS){
                handle_power_action(2); // power off
            } else if(held>DEBOUNCE_MS){
                handle_power_action(0); // display toggle
            }
            power_down=0;
        }

        // ── Vol+ (right): brightness+ ──
        if(read_btn(BTN_UP)) {
            if(up_down==0){up_down=now;up_handled=false;}
            else if(!up_handled&&pdTICKS_TO_MS(now-up_down)>DEBOUNCE_MS)
                {handle_vol_up();up_handled=true;}
        } else {up_down=0;}

        // ── Vol- (left): brightness- ──
        if(read_btn(BTN_DOWN)) {
            if(down_down==0){down_down=now;down_handled=false;}
            else if(!down_handled&&pdTICKS_TO_MS(now-down_down)>DEBOUNCE_MS)
                {handle_vol_down();down_handled=true;}
        } else {down_down=0;}

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t buttons_init(void) {
    // Configure button GPIOs with pull-ups (active low)
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BTN_POWER) | (1ULL << BTN_UP) | (1ULL << BTN_DOWN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    xTaskCreate(button_task, "buttons", 3072, NULL, 3, NULL);
    ESP_LOGI(TAG, "buttons initialized (PWR=5, UP=4, DOWN=0)");
    return ESP_OK;
}
