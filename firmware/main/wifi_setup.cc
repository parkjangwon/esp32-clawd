#include "wifi_setup.h"
#include "display.h"
#include <esp_wifi.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "wifi_setup";
static bool g_active = false;

// Tiny font
static const uint8_t *f8(char c) {
    static const uint8_t d0[]={0x3C,0x66,0x6E,0x7E,0x76,0x66,0x66,0x3C};
    static const uint8_t d1[]={0x18,0x38,0x18,0x18,0x18,0x18,0x18,0x7E};
    static const uint8_t d2[]={0x3C,0x66,0x06,0x0C,0x18,0x30,0x60,0x7E};
    static const uint8_t d3[]={0x3C,0x66,0x06,0x1C,0x06,0x06,0x66,0x3C};
    static const uint8_t d4[]={0x0C,0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C};
    static const uint8_t d5[]={0x7E,0x60,0x7C,0x06,0x06,0x06,0x66,0x3C};
    static const uint8_t d6[]={0x3C,0x66,0x60,0x7C,0x66,0x66,0x66,0x3C};
    static const uint8_t d7[]={0x7E,0x06,0x06,0x0C,0x18,0x18,0x30,0x30};
    static const uint8_t d8[]={0x3C,0x66,0x66,0x3C,0x66,0x66,0x66,0x3C};
    static const uint8_t d9[]={0x3C,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C};
    static const uint8_t dc[]={0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00};
    static const uint8_t dd[]={0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00};
    static const uint8_t dp[]={0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18};
    static const uint8_t ds[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    switch(c){
    case '0':return d0;case '1':return d1;case '2':return d2;case '3':return d3;
    case '4':return d4;case '5':return d5;case '6':return d6;case '7':return d7;
    case '8':return d8;case '9':return d9;case ':':return dc;case '-':return dd;
    case '.':return dp;case ' ':return ds;
    default:return nullptr;
    }
}
static void dc(uint16_t*b,int x,int y,char c,uint16_t cl){const uint8_t*g=f8(c);if(!g)return;for(int r=0;r<8;r++){uint8_t bits=g[r];for(int col=0;col<8;col++){if(bits&(1<<(7-col))){int px=x+col,py=y+r;if(px>=0&&px<240&&py>=0&&py<240)b[py*240+px]=cl;}}}}
static void ds(uint16_t*b,int x,int y,const char*s,uint16_t cl){for(;*s;s++){dc(b,x,y,*s,cl);x+=8;}}
static uint16_t mc(uint8_t r,uint8_t g,uint8_t b){return __builtin_bswap16(((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3));}

esp_err_t wifi_setup_start(void) {
    ESP_LOGI(TAG,"=== WIFI SETUP START ===");
    if(g_active){ESP_LOGW(TAG,"already active");return ESP_OK;}
    g_active=true;

    uint16_t*buf=(uint16_t*)heap_caps_malloc(240*240*2,MALLOC_CAP_DMA);
    if(!buf){ESP_LOGE(TAG,"malloc fail");g_active=false;return ESP_ERR_NO_MEM;}
    ESP_LOGI(TAG,"buf allocated");

    uint16_t bg=mc(26,28,40),wh=mc(255,255,255),gr=mc(0,255,0),ye=mc(255,200,0);

    // Draw test screen — NO WiFi scan
    for(int i=0;i<240*240;i++)buf[i]=bg;
    ds(buf,10,10,"=== WiFi Setup ===",ye);
    ds(buf,10,35,"SSID: Clawd-Setup",wh);
    ds(buf,10,55,"IP: 192.168.4.1",gr);
    ds(buf,10,80,"Connect phone to",wh);
    ds(buf,10,100,"this WiFi, then",wh);
    ds(buf,10,120,"open browser to",wh);
    ds(buf,10,140,"192.168.4.1",gr);
    ds(buf,10,170,"Press Vol+ to exit",ye);
    display_draw_bitmap(0,0,240,240,buf);
    ESP_LOGI(TAG,"screen drawn, waiting...");

    // Stay on screen
    for(int i=0;i<100;i++){
        vTaskDelay(pdMS_TO_TICKS(100));
        if(!g_active)break;
    }

    ESP_LOGI(TAG,"setup exit");
    free(buf);
    g_active=false;
    return ESP_OK;
}

bool wifi_setup_is_active(void) {return g_active;}

void wifi_setup_btn_up(void) {ESP_LOGI(TAG,"btn_up");}
void wifi_setup_btn_down(void) {ESP_LOGI(TAG,"btn_down");}
void wifi_setup_btn_select(void) {ESP_LOGI(TAG,"btn_select");}

bool wifi_has_stored_creds(void) {
    nvs_handle_t h;
    if(nvs_open("wifi",NVS_READONLY,&h)!=ESP_OK) return false;
    size_t len;bool ok=(nvs_get_str(h,"ssid",NULL,&len)==ESP_OK);
    nvs_close(h);return ok;
}
void wifi_clear_creds(void) {
    nvs_handle_t h;
    if(nvs_open("wifi",NVS_READWRITE,&h)==ESP_OK){nvs_erase_all(h);nvs_commit(h);nvs_close(h);}
}
esp_err_t wifi_save_creds(const char *ssid, const char *pass) {
    nvs_handle_t h;
    esp_err_t r=nvs_open("wifi",NVS_READWRITE,&h);
    if(r!=ESP_OK)return r;
    nvs_set_str(h,"ssid",ssid);nvs_set_str(h,"pass",pass);
    nvs_commit(h);nvs_close(h);
    return ESP_OK;
}
