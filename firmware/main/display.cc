#include "display.h"
#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_master.h>
#include <driver/spi_common.h>
#include <driver/ledc.h>

static const char *TAG = "display";

// Waveshare ESP32-S3-Touch-LCD-1.54 pin mapping
#define PIN_LCD_CS  21
#define PIN_LCD_DC  45
#define PIN_LCD_RST 40
#define PIN_LCD_MOSI 39
#define PIN_LCD_CLK 38
#define PIN_LCD_BL  46

#define LCD_HOST    SPI2_HOST
#define LCD_WIDTH   240
#define LCD_HEIGHT  240
#define SPI_CLOCK_HZ 80 * 1000 * 1000  // 80MHz

static esp_lcd_panel_handle_t panel = NULL;

esp_err_t display_init(void) {
    ESP_LOGI(TAG, "Initializing display (ST7789, 240x240)");

    // 1. Initialize SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2 + 8,
    };
    esp_err_t ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SPI bus initialized (host=%d)", LCD_HOST);

    // 2. Create panel IO handle (SPI)
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = PIN_LCD_CS,
        .dc_gpio_num = PIN_LCD_DC,
        .spi_mode = 3,
        .pclk_hz = SPI_CLOCK_HZ,
        .trans_queue_depth = 1,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel IO init failed: %s", esp_err_to_name(ret));
        spi_bus_free(LCD_HOST);
        return ret;
    }
    ESP_LOGI(TAG, "Panel IO created");

    // 3. Create ST7789 panel
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel create failed: %s", esp_err_to_name(ret));
        esp_lcd_panel_io_del(io_handle);
        spi_bus_free(LCD_HOST);
        return ret;
    }
    ESP_LOGI(TAG, "ST7789 panel created");

    // 4. Initialize panel
    ret = esp_lcd_panel_reset(panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(ret));
        esp_lcd_panel_del(panel);
        esp_lcd_panel_io_del(io_handle);
        spi_bus_free(LCD_HOST);
        return ret;
    }
    ret = esp_lcd_panel_init(panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        esp_lcd_panel_del(panel);
        esp_lcd_panel_io_del(io_handle);
        spi_bus_free(LCD_HOST);
        return ret;
    }
    ESP_LOGI(TAG, "Panel initialized");

    // 5. Configure ST7789: color inversion, mirror/swap
    esp_lcd_panel_invert_color(panel, true);
    esp_lcd_panel_swap_xy(panel, false);
    esp_lcd_panel_mirror(panel, false, false);

    // 6. Turn display on
    esp_lcd_panel_disp_on_off(panel, true);
    ESP_LOGI(TAG, "Display turned on");

    // 7. Initialize backlight PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(ret));
    }

    ledc_channel_config_t ledc_channel = {
        .gpio_num = PIN_LCD_BL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 512,  // 50% initially
        .hpoint = 0,
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Backlight PWM initialized (50%%)");
    }

    ESP_LOGI(TAG, "Display initialization complete");
    return ESP_OK;
}

esp_err_t display_draw_bitmap(int x_start, int y_start, int x_end, int y_end, const void *color_data) {
    if (!panel) return ESP_ERR_INVALID_STATE;
    return esp_lcd_panel_draw_bitmap(panel, x_start, y_start, x_end, y_end, color_data);
}

void display_set_backlight(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint32_t duty = (percent * 1023) / 100;  // 10-bit resolution
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void display_clear(uint16_t color) {
    size_t buf_size = LCD_WIDTH * LCD_HEIGHT * 2;
    void *buf = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate clear buffer");
        return;
    }
    uint16_t *pixels = (uint16_t *)buf;
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        pixels[i] = color;
    }
    display_draw_bitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, buf);
    free(buf);
}
