#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"

#define WIFI_SSID "REDE_ECG_MONITOR"
#define WIFI_PASS "123456"

void wifi_init_ap(const char *ssid, const char *pass, uint8_t channel, uint8_t max_conn, bool hidden);

#ifdef __cplusplus
}
#endif