#pragma once
#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start WiFi setup UI (scan → select → keyboard password → connect)
esp_err_t wifi_setup_start(void);

// True while setup UI is active (blocks normal operation)
bool wifi_setup_is_active(void);

// Button events for setup navigation
void wifi_setup_btn_up(void);     // Vol+
void wifi_setup_btn_down(void);   // Vol-
void wifi_setup_btn_select(void); // Center press

// Check/clear stored credentials
bool wifi_has_stored_creds(void);
void wifi_clear_creds(void);
esp_err_t wifi_save_creds(const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif
