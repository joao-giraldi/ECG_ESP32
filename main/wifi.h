#include "esp_wifi.h"

#define WIFI_SSID "REDE_ESP_ECG"
#define WIFI_PASS "123456"

void ap_wifi_config(void);
void wifi_init_ap(const char *ssid, const char *pass, uint8_t channel, uint8_t max_conn, bool hidden);