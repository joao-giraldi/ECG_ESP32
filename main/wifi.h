#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"

#define WIFI_SSID "ECG-ESP32"
#define WIFI_PASS "12345678"

void wifi_init_ap(const char *ssid, const char *pass, uint8_t channel, uint8_t max_conn, bool hidden);

#ifdef __cplusplus
}
#endif