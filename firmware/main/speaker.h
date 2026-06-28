#pragma once
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t speaker_init(void);
void speaker_beep(int freq_hz, int duration_ms);    // simple tone
void speaker_success(void);  // ascending beeps
void speaker_error(void);    // low buzz
void speaker_notify(void);   // single beep

#ifdef __cplusplus
}
#endif
