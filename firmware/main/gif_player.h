#pragma once
#include <esp_err.h>

/**
 * Initialize the GIF player and mount SPIFFS.
 * Must be called before gif_player_play().
 */
esp_err_t gif_player_init(void);

/**
 * Start playing animation for the given state.
 * Stops any previously playing animation gracefully.
 * @param state  State name (e.g., "thinking", "idle")
 */
esp_err_t gif_player_play(const char *state);

/**
 * Stop any playing animation and clear display.
 */
void gif_player_stop(void);
