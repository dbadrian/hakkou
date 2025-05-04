#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

namespace hakkou {

esp_err_t http_server_init(void);
esp_err_t http_server_stop(void);
esp_err_t http_server_register_uri_handler(const httpd_uri_t& uri_handler);

}  // namespace hakkou