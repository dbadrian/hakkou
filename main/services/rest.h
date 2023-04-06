#pragma once

/* HTTP Restful API Server
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "event.h"

#include "cJSON.h"

#include "esp_chip_info.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#include <fcntl.h>
#include <string.h>
#include <algorithm>

namespace hakkou {
static const char* REST_TAG = "esp-rest";

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
  char base_path[ESP_VFS_PATH_MAX + 1];
  char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) \
  (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t* req,
                                            const char* filepath) {
  const char* type = "text/plain";
  if (CHECK_FILE_EXTENSION(filepath, ".html")) {
    type = "text/html";
  } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
    type = "application/javascript";
  } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
    type = "text/css";
  } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
    type = "image/png";
  } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
    type = "image/x-icon";
  } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
    type = "text/xml";
  }
  return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t* req) {
  char filepath[FILE_PATH_MAX];

  rest_server_context_t* rest_context = (rest_server_context_t*)req->user_ctx;
  strlcpy(filepath, rest_context->base_path, sizeof(filepath));
  if (req->uri[strlen(req->uri) - 1] == '/') {
    strlcat(filepath, "/index.html", sizeof(filepath));
  } else {
    strlcat(filepath, req->uri, sizeof(filepath));
  }
  int fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to read existing file");
    return ESP_FAIL;
  }

  set_content_type_from_file(req, filepath);

  char* chunk = rest_context->scratch;
  ssize_t read_bytes;
  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
    if (read_bytes == -1) {
      ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
    } else if (read_bytes > 0) {
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
        close(fd);
        ESP_LOGE(REST_TAG, "File sending failed!");
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "Failed to send file");
        return ESP_FAIL;
      }
    }
  } while (read_bytes > 0);
  /* Close file after sending complete */
  close(fd);
  ESP_LOGI(REST_TAG, "File sending complete");
  /* Respond with an empty chunk to signal HTTP response completion */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  cJSON* root = cJSON_CreateObject();
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  cJSON_AddStringToObject(root, "version", IDF_VER);
  cJSON_AddNumberToObject(root, "cores", chip_info.cores);
  const char* sys_info = cJSON_Print(root);
  httpd_resp_sendstr(req, sys_info);
  free((void*)sys_info);
  cJSON_Delete(root);
  return ESP_OK;
}

char* parse_request_to_string(httpd_req_t* req) {
  int total_len = req->content_len;
  int cur_len = 0;
  char* buf = ((rest_server_context_t*)(req->user_ctx))->scratch;
  int received = 0;
  if (total_len >= SCRATCH_BUFSIZE) {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "content too long");
    return nullptr;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0) {
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to post");  // TODO: message
      return nullptr;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  return buf;
}

/* Simple handler for light brightness control */
static esp_err_t gui_event_post_handler(httpd_req_t* req) {
  /*
    {
      "gui_cmd": (1) "UP" or "DOWN", "OK", "LEFT", "RIGHT", "ESC"
    }

    ESC: -1
    OK: 0
    UP: 1
    DOWN: 2
    LEFT: 3
    RIGHT: 4
  */

  char* buf = parse_request_to_string(req);
  if (buf == nullptr) {
    HERROR("Couldn't parse header to buffer");
    return ESP_FAIL;
  }
  HDEBUG("%s", buf);

  cJSON* root = cJSON_Parse(buf);
  cJSON* guicmd = cJSON_GetObjectItemCaseSensitive(root, "gui_cmd");
  if (cJSON_IsNumber(guicmd)) {
    HINFO("GOT GUICMD %d", guicmd->valueint);
    switch (guicmd->valueint) {
      case -1: {
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_ESC});
      } break;
      case 0: {
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_OK});
      } break;
      case 1: {
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_UP});
      } break;
      case 2: {
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_DOWN});
      } break;
      case 3: {
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_LEFT});
      } break;
      case 4: {
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_RIGHT});
      } break;

      default: {
        HDEBUG("[REST] Got an invalid gui command integer");
      } break;
    }
  }
  cJSON_Delete(root);
  httpd_resp_sendstr(req, "Posted successfully");
  return ESP_OK;
}

httpd_handle_t start_rest_server(const char* base_path) {
  if (!base_path) {
    ESP_LOGE(REST_TAG, "Wrong base path");
    return nullptr;
  }
  rest_server_context_t* rest_context = static_cast<rest_server_context_t*>(
      calloc(1, sizeof(rest_server_context_t)));
  if (!rest_context) {
    ESP_LOGE(REST_TAG, "No memory for rest context");
    return nullptr;
  }
  strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 10;
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(REST_TAG, "Starting HTTP Server");
  if (!httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGE(REST_TAG, "Start rest server failed");
    free(rest_context);
    return nullptr;
  }

  /* URI handler for light brightness control */
  httpd_uri_t gui_cmd_handler = {.uri = "/api/v1/gui_cmd",
                                 .method = HTTP_POST,
                                 .handler = gui_event_post_handler,
                                 .user_ctx = rest_context};
  httpd_register_uri_handler(server, &gui_cmd_handler);

  /* URI handler for fetching system info */
  httpd_uri_t system_info_get_uri = {.uri = "/api/v1/system/info",
                                     .method = HTTP_GET,
                                     .handler = system_info_get_handler,
                                     .user_ctx = rest_context};
  httpd_register_uri_handler(server, &system_info_get_uri);

  // NEXT PART IS LAST, DONT MODIFY
  /* URI handler for getting web server files */
  httpd_uri_t common_get_uri = {.uri = "/*",
                                .method = HTTP_GET,
                                .handler = rest_common_get_handler,
                                .user_ctx = rest_context};
  httpd_register_uri_handler(server, &common_get_uri);

  return server;
}

// static void initialise_mdns(void) {
//   mdns_init();
//   mdns_hostname_set(CONFIG_MDNS_HOST_NAME);
//   mdns_instance_name_set(MDNS_INSTANCE);

//   mdns_txt_item_t serviceTxtData[] = {{"board", "esp32"}, {"path", "/"}};

//   ESP_ERROR_CHECK(
//       mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80,
//       serviceTxtData,
//                        sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
// }

esp_err_t init_fs(void) {
  esp_vfs_spiffs_conf_t conf = {.base_path = CONFIG_WEB_MOUNT_POINT,
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = false};
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      HERROR("Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      HERROR("Failed to find SPIFFS partition");
    } else {
      HERROR("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    HERROR("Failed to get SPIFFS partition information (%s)",
           esp_err_to_name(ret));
  } else {
    HINFO("Partition size: total: %d, used: %d", total, used);
  }
  return ESP_OK;
}

void rest_initialize() {
  init_fs();
  start_rest_server(CONFIG_WEB_MOUNT_POINT);

  // some inits for the rest server
  // initialise_mdns();
  // netbiosns_init();
  // netbiosns_set_name(CONFIG_MDNS_HOST_NAME);
}

}  // namespace hakkou