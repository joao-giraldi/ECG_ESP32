#include "mdns.h"
#include "esp_log.h"

static const char *TAG = "mdns_app";

void mdns_app_start(const char *hostname) {
    ESP_LOGI(TAG, "Inicializando mDNS como '%s.local'", hostname);
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_ERROR_CHECK(mdns_instance_name_set("ECG ESP32"));

    // Anuncia o seu servidor HTTP na porta 80
    mdns_txt_item_t txt[] = {
        { "board", "esp32" },
        { "path",  "/" }
    };
    ESP_ERROR_CHECK(mdns_service_add(/*instance*/NULL, "_http", "_tcp", 80, txt, 2));
    // Se quiser personalizar o "display name" só do HTTP:
    // mdns_service_instance_name_set("_http", "_tcp", "ECG Web");
}
