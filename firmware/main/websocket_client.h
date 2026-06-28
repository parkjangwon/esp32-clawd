#pragma once
#include <esp_err.h>
#include <functional>
#include <string>

using StateCallback = std::function<void(const std::string &state)>;

esp_err_t ws_client_start(const char *url, StateCallback on_state);
void ws_client_stop();
