#pragma once
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

esp_err_t wifi_init_sta(void);
extern SemaphoreHandle_t wifi_connected_sem;
