#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "http";

static const char INDEX_HTML[] =
"<!doctype html><html lang='pt-br'><head>"
"<meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>ECG - Teste</title>"
"<style>body{display:grid;min-height:100svh;place-items:center;background:#111;font-family:system-ui}"
"h1{color:red;font-size:clamp(24px,6vw,54px);margin:0}</style>"
"</head><body><h1>olá mundo!</h1></body></html>";

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;   // libera handlers antigos se faltar RAM
    config.server_port = 80;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root);
        ESP_LOGI(TAG, "HTTP em http://192.168.4.1");
    } else {
        ESP_LOGE(TAG, "Falha ao iniciar HTTP server");
    }
    return server;
}

void stop_webserver(httpd_handle_t server) {
    if (server) httpd_stop(server);
}
