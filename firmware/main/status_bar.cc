#include "status_bar.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/apps/sntp.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "status_bar";

#define BAR_H  24
#define BAR_W  240

// ─── Font: 8x8 glyphs (MSB = top row) ────────────────────────────────────
static const uint8_t *font(char c) {
    static const uint8_t g0[]={0x3C,0x66,0x6E,0x7E,0x76,0x66,0x66,0x3C};
    static const uint8_t g1[]={0x18,0x38,0x18,0x18,0x18,0x18,0x18,0x7E};
    static const uint8_t g2[]={0x3C,0x66,0x06,0x0C,0x18,0x30,0x60,0x7E};
    static const uint8_t g3[]={0x3C,0x66,0x06,0x1C,0x06,0x06,0x66,0x3C};
    static const uint8_t g4[]={0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C};
    static const uint8_t g5[]={0x7E,0x60,0x7C,0x06,0x06,0x06,0x66,0x3C};
    static const uint8_t g6[]={0x3C,0x66,0x60,0x7C,0x66,0x66,0x66,0x3C};
    static const uint8_t g7[]={0x7E,0x06,0x06,0x0C,0x18,0x18,0x30,0x30};
    static const uint8_t g8[]={0x3C,0x66,0x66,0x3C,0x66,0x66,0x66,0x3C};
    static const uint8_t g9[]={0x3C,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C};
    static const uint8_t gc[]={0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}; // :
    static const uint8_t gp[]={0x63,0x66,0x0C,0x18,0x30,0x66,0xC6,0x00}; // %
    static const uint8_t gd[]={0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}; // -
    static const uint8_t gdot[]={0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18};// .
    static const uint8_t gsp[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // space
    switch(c){
    case '0':return g0;case '1':return g1;case '2':return g2;case '3':return g3;
    case '4':return g4;case '5':return g5;case '6':return g6;case '7':return g7;
    case '8':return g8;case '9':return g9;case ':':return gc;case '%':return gp;
    case '-':return gd;case '.':return gdot;case ' ':return gsp;
    default:return nullptr;
    }
}

static void draw_char(uint16_t *b, int x, int y, char c, uint16_t color) {
    const uint8_t *g = font(c);
    if(!g) return;
    for(int r=0;r<8;r++) {
        uint8_t bits=g[r];
        for(int col=0;col<8;col++) {
            if(bits&(1<<(7-col))){
                int px=x+col, py=y+r;
                if(px>=0&&px<BAR_W&&py>=0&&py<BAR_H) b[py*BAR_W+px]=color;
            }
        }
    }
}

static void draw_str(uint16_t *b, int x, int y, const char *s, uint16_t color) {
    for(;*s;s++){draw_char(b,x,y,*s,color);x+=8;if(x+8>BAR_W)break;}
}

// ─── WiFi bars (phone-style 4-bar icon) ─────────────────────────────────
static void draw_wifi_bars(uint16_t *b, int x, int y, int rssi, uint16_t color) {
    // 4 bars of heights 2,5,8,11 pixels, width 3px each, 1px gap
    // Strongest bar = 4 filled, weakest = 1 filled
    int bars;
    if(rssi>-50) bars=4;
    else if(rssi>-67) bars=3;
    else if(rssi>-80) bars=2;
    else bars=1;

    for(int i=0;i<4;i++) {
        int h=2+i*3; // bar heights: 2,5,8,11
        int bx=x+i*4;
        int filled=(i<bars);
        for(int row=0;row<12;row++){
            for(int col=0;col<3;col++){
                int px=bx+col, py=y+11-row;
                bool on=(row<h && filled);
                if(px>=0&&px<BAR_W&&py>=0&&py<BAR_H) {
                    b[py*BAR_W+px]=on?color:__builtin_bswap16(0x3186); // dark for empty bars
                }
            }
        }
    }
}

// ─── Battery icon ───────────────────────────────────────────────────────
static void draw_battery_icon(uint16_t *b, int x, int y, int pct, uint16_t color) {
    int bw=20, bh=10;
    uint16_t dark=__builtin_bswap16(0x2106);
    // Body outline
    for(int row=0;row<bh;row++){
        for(int col=0;col<bw;col++){
            int px=x+col, py=y+row;
            if(px>=BAR_W||py>=BAR_H) continue;
            bool edge=(row==0||row==bh-1||col==0||col==bw-1);
            b[py*BAR_W+px]=edge?color:dark;
        }
    }
    // Fill level
    int fill_w=((bw-2)*pct)/100;
    for(int row=1;row<bh-1;row++){
        for(int col=1;col<=fill_w;col++){
            int px=x+col, py=y+row;
            if(px<BAR_W&&py<BAR_H) b[py*BAR_W+px]=color;
        }
    }
    // Cap
    for(int row=2;row<8;row++){
        for(int col=0;col<2;col++){
            int px=x+bw+col, py=y+row;
            if(px<BAR_W&&py<BAR_H) b[py*BAR_W+px]=color;
        }
    }
}

// ─── State ──────────────────────────────────────────────────────────────
static char g_time_str[6]="--:--";
static int  g_bat_pct = 100;
static bool g_bat_present = false;

// ADC oneshot handle (like xiaozhi PowerManager)
static adc_oneshot_unit_handle_t g_adc = NULL;
static adc_cali_handle_t g_adc_cali = NULL;
static bool g_do_calibration = false;

esp_err_t status_bar_init(void) {
    // Battery enable pin GPIO2 (matching xiaozhi PowerManager config)
    gpio_config_t pwr_cfg = {
        .pin_bit_mask = 1ULL << GPIO_NUM_2,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&pwr_cfg);
    gpio_set_level(GPIO_NUM_2, 1);  // power on battery measurement

    // Charging detect GPIO3 with pull-up
    gpio_config_t chg_cfg = {
        .pin_bit_mask = 1ULL << GPIO_NUM_3,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&chg_cfg);

    // ADC oneshot init (GPIO1 = ADC1_CH0)
    adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_cfg, &g_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_adc, ADC_CHANNEL_0, &chan_cfg));

    // Calibration
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_0,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &g_adc_cali) == ESP_OK) {
        g_do_calibration = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    }

    setenv("TZ","KST-9",1);
    tzset();
    ESP_LOGI(TAG,"init ok");
    return ESP_OK;
}

void status_bar_start_sntp(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*)"pool.ntp.org");
    sntp_init();
    ESP_LOGI(TAG,"sntp started");
}


void status_bar_update_time(void) {
    time_t now;
    time(&now);
    if(now>1000000000){
        struct tm ti;
        localtime_r(&now,&ti);
        snprintf(g_time_str,sizeof(g_time_str),"%02d:%02d",ti.tm_hour,ti.tm_min);
    }

    // Read battery using oneshot API (like xiaozhi PowerManager)
    if (g_adc) {
        int raw = 0, voltage_mv = 0;
        adc_oneshot_read(g_adc, ADC_CHANNEL_0, &raw);
        g_bat_present = (raw > 100);

        if (g_bat_present && g_do_calibration) {
            adc_cali_raw_to_voltage(g_adc_cali, raw, &voltage_mv);
            // voltage_mv is at ADC pin. Actual battery = ~3x due to 2:1 divider
            float bat_v = (voltage_mv / 1000.0f) * 3.0f;

            // Same thresholds as xiaozhi
            if (bat_v < 3.52f)        g_bat_pct = 1;
            else if (bat_v < 3.64f)   g_bat_pct = 20;
            else if (bat_v < 3.76f)   g_bat_pct = 40;
            else if (bat_v < 3.88f)   g_bat_pct = 60;
            else if (bat_v < 4.00f)   g_bat_pct = 80;
            else                       g_bat_pct = 100;
        } else if (g_bat_present) {
            // No calibration: rough estimate from raw value
            // raw range ~1600(3.3V) to ~2850(4.2V) at ADC_ATTEN_DB_12
            if (raw < 1600)           g_bat_pct = 1;
            else if (raw < 2850)      g_bat_pct = 20 + (raw - 1600) * 60 / 1250;
            else                       g_bat_pct = 100;
        }
    }
}

// ─── Main overlay ────────────────────────────────────────────────────────
void status_bar_overlay(uint16_t *buf, int width, int height) {
    if(!buf||width<BAR_W||height<BAR_H) return;

    uint16_t bg    = __builtin_bswap16(0x2106); // bar bg (slightly lighter navy)
    uint16_t white = __builtin_bswap16(0xFFFF);
    uint16_t green = __builtin_bswap16(0x07E0);
    uint16_t gold  = __builtin_bswap16(0xFF80); // amber/gold
    uint16_t red   = __builtin_bswap16(0xF800);
    uint16_t line  = __builtin_bswap16(0x4208); // dim line

    // Fill bar bg
    for(int i=0;i<BAR_W*BAR_H;i++) buf[i]=bg;
    // Bottom separator
    for(int x=0;x<BAR_W;x++) buf[(BAR_H-1)*BAR_W+x]=line;

    int cy=(BAR_H-12)/2; // vertical center for icons

    // Read WiFi + battery
    wifi_ap_record_t ap;
    int rssi=-99;
    if(esp_wifi_sta_get_ap_info(&ap)==ESP_OK) rssi=ap.rssi;
    status_bar_update_time();

    // ── Left: Time (24h KST) ──
    draw_str(buf,4,cy+2,g_time_str,white);

    // ── Right: % text → battery icon ──
    if(g_bat_present){
        bool charging = (gpio_get_level(GPIO_NUM_3) == 0);
        uint16_t bc = charging ? green : ((g_bat_pct>20)?green:red);
        // Percentage text first (right side)
        char bs[8];
        snprintf(bs,sizeof(bs),"%d%%",g_bat_pct);
        draw_str(buf,BAR_W-50,cy+2,bs,bc);
        // Battery icon after text
        draw_battery_icon(buf,BAR_W-26,cy,g_bat_pct,bc);
    } else {
        draw_str(buf,BAR_W-34,cy+2,"---",green);
    }

    // ── WiFi bars with spacing ──
    uint16_t wc=(rssi>-80)?green:((rssi>-90)?gold:red);
    draw_wifi_bars(buf,BAR_W-86,cy,rssi,wc);
}
