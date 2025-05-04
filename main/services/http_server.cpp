#include "esp_http_server.h"
#include "esp_log.h"

#include "event.h"
#include "internal_types.h"

#include "http_server.h"

namespace hakkou {

namespace {
static const char* TAG = "http_server";
static httpd_handle_t server = NULL;
static EventHandle handle;

static constexpr int MAX_HANDLERS = 20;
static httpd_uri_t registered_handlers[MAX_HANDLERS];
static std::uint16_t registered_handlers_count = 0;
}  // namespace

CallbackResponse on_wifi_event(Event event, void* listener) {
  switch (event.wifi_event) {
    case WifiEvent::CONNECTED: {
      if (server == nullptr) {
        ESP_LOGI(TAG, "Starting HTTP server");

        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        if (httpd_start(&server, &config) == ESP_OK) {
          // httpd_register_uri_handler(server, &update_uri);
          // httpd_register_uri_handler(server, &status_uri);

          for(int i = 0; i < registered_handlers_count; i++) {
            ESP_LOGI(TAG, "Registering server uri");
            httpd_register_uri_handler(server, &registered_handlers[i]);
          }
          ESP_LOGI(TAG, "HTTP server started");
        } else {
          ESP_LOGE(TAG, "Failed to start HTTP server");
          server = NULL;
        }
      }
    } break;
    case WifiEvent::CONNECTING: {
    } break;
    case WifiEvent::DISCONNECTED: {
      if (server != nullptr) {
        httpd_stop(server);
      }
    } break;
    default:
      break;
  }

  return CallbackResponse::Continue;
}

esp_err_t http_server_init(void) {
  // register callbacks with the event subsystem
  auto h = event_register(EventType::WIFI, nullptr, on_wifi_event);
  if (!h) {
    HFATAL("Couldn't register heater duty callback!");
  }
  handle = h.value();
  return ESP_OK;
}

esp_err_t http_server_stop(void) {
  // TODO: deregister the event callback
  event_unregister(handle);
  if (server != nullptr) {
    httpd_stop(server);
  }
  server = NULL;
  return ESP_OK;
}

esp_err_t http_server_register_uri_handler(const httpd_uri_t& uri_handler) {
  if (registered_handlers_count >= MAX_HANDLERS) {
    return ESP_ERR_NO_MEM;
  } 

  registered_handlers[registered_handlers_count++] = uri_handler;
  return ESP_OK;
}

}  // namespace hakkou