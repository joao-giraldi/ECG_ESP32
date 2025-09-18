#pragma once
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

void web_register_sd_api(httpd_handle_t server);

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

#ifdef __cplusplus
}
#endif
