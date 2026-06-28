#pragma once
#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start SoftAP + web portal mode for WiFi setup
esp_err_t wifi_ap_start(void);

// Check if currently in AP/config mode
bool wifi_ap_is_active(void);

// Return true if NVS has stored WiFi credentials
bool wifi_has_stored_creds(void);

// Clear stored credentials (to force reconfiguration)
void wifi_clear_creds(void);

// Save new credentials to NVS
esp_err_t wifi_save_creds(const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif
