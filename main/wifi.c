

// void ap_wifi_config(void) {
//     //esp_wifi_init(WIFI_INIT_CONFIG_DEFAULT);

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_crete_default());
//     esp_netif_create_default();

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVEN_ANY_ID, &wifi_event_handler, NULL, NULL));

//     wifi_config_t wifi_config = {
//         .ap = {
//             .ssid = WIFI_SSID,
//             .ssid_len = strlen(WIFI_SSID),
//             .channel = CONFIG_ESP_WIFI_CHANNEL,
//             .password = WIFI_PASS,
//             .max_connection = CONFIG_ESP_MAX_STA_CONN,
//             .pmfcfg = {
//                 .required = true,
//             },
//         }
//     };

//     if(strlen(WIFI_PASS) == 0) {
//         wifi_config.ap.authmode = WIFI_AUTH_OPEN;
//     }

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s,
//              WIFI_SSID, WIFI_PASS);
// }

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"

static const char *TAG = "wifi_ap";

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *e = (wifi_event_ap_staconnected_t *)data;
        ESP_LOGI(TAG, "STA conectou");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *e = (wifi_event_ap_stadisconnected_t *)data;
        ESP_LOGI(TAG, "STA desconectou");
    }
}

void wifi_init_ap(const char *ssid, const char *pass, uint8_t channel, uint8_t max_conn, bool hidden) {
    // 1) NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // 2) Stack de rede + loop de eventos
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap(); // cria a interface de AP (DHCP server por padrão)

    // 3) Wi-Fi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 4) Handlers para eventos de AP
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    // 5) Configuração do AP
    wifi_config_t wifi_config = {0};
    // SSID e senha
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ssid);

    if (pass && pass[0] != '\0') {
        strncpy((char *)wifi_config.ap.password, pass, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK; // use WPA3/WPA2 conforme suporte do seu IDF/cliente
    } else {
        wifi_config.ap.password[0] = '\0';
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // Canal, máximo de conexões, SSID oculto ou não
    wifi_config.ap.channel = channel;
    wifi_config.ap.max_connection = max_conn;
    wifi_config.ap.ssid_hidden = hidden ? 1 : 0;

    // PMF (geralmente ok manter "capable" p/ compatibilidade)
    wifi_config.ap.pmf_cfg.capable = true;
    wifi_config.ap.pmf_cfg.required = false;

    // 6) Define modo AP, aplica config e inicia
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Por padrão, a interface AP usa IP 192.168.4.1/24 e DHCP server habilitado.
    // Se quiser IP estático custom, desabilite DHCP e set_ip_info na esp-netif aqui.

    ESP_LOGI(TAG, "SoftAP ativo: SSID='%s' canal=%u auth=%d max_conn=%u hidden=%d",
             ssid, channel, (int)wifi_config.ap.authmode, (unsigned)max_conn, (int)wifi_config.ap.ssid_hidden);
    ESP_LOGI(TAG, "Conecte-se e acesse http://192.168.4.1/ (padrão)");
}