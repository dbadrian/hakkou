#pragma once

#include "event.h"
#include "gui.h"
#include "logger.h"

// TODO: Move defines to global defiens
#define OTA_PORT 8050
#define BUF_SIZE 1024
#define HASH_SIZE 32

// static const char *TAG = "OTA_SERVER";

// #include "esp_system.h"
#include "esp_ota_ops.h"
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"
#include "lwip/sockets.h"
#include "mbedtls/md.h"

#include <cstring>

namespace hakkou {
// static constexpr char MAGIC_HEADER[] = "HakkouOTA!!!";
static constexpr char MAGIC_HEADER[] = {'H', 'a', 'k', 'k', 'o', 'u',
                                        'O', 'T', 'A', '!', '!', '!'};

struct OTAHeader {
  char magic[sizeof(MAGIC_HEADER)];  // e.g., 'OTAv' but not escaped \0...
  uint32_t firmware_len;
  uint8_t sha256[32];

  bool is_valid() const {
    return memcmp(magic, MAGIC_HEADER, sizeof(MAGIC_HEADER)) == 0;
  }
};

void ota_server_task(void* context) {
  /*
      This tasks listens for the upload of an OTA firmware.
  */
  OTAScreen screen;

  event_post(Event{.event_type = EventType::ScreenUpdate,
                   .screen_data = screen.data()});

  int ret = 0;
  int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  // ASKA_ASSERT_MSG(listen_sock >= 0, "Socket creation failed");

  sockaddr_in server_addr{.sin_family = AF_INET,
                          .sin_port = htons(OTA_PORT),
                          .sin_addr = {htonl(INADDR_ANY)}};
  ret = bind(listen_sock, reinterpret_cast<sockaddr*>(&server_addr),
             sizeof(server_addr));
  // ASKA_ASSERT_MSG(ret == 0, "Socket bind failed");

  ret = listen(listen_sock, 1);
  // ASKA_ASSERT_MSG(ret == 0, "Socket listen failed");

  HINFO("Waiting for OTA connection on port %d...", OTA_PORT);

  sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_sock =
      accept(listen_sock, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
  // ASKA_ASSERT_MSG(client_sock >= 0, "Accept failed");

  HINFO("Client connected");

  OTAHeader header{};
  int header_received = 0;
  while (header_received < sizeof(OTAHeader)) {
    ret = recv(client_sock, reinterpret_cast<char*>(&header) + header_received,
               sizeof(header) - header_received, 0);
    // ASKA_ASSERT_MSG(ret > 0, "Header receive failed");
    header_received += ret;
  }
  // ASKA_ASSERT_MSG(ret >= sizeof(OTAHeader), "Header receive failed");
  // ASKA_ASSERT_MSG(header.is_valid(), "Header received is not valid");

  HINFO("OTA Header OK. Receiving %lu bytes...", header.firmware_len);

  const esp_partition_t* update_partition =
      esp_ota_get_next_update_partition(nullptr);
  // ASKA_ASSERT_MSG(update_partition, "No OTA partition found");

  esp_ota_handle_t ota_handle;
  esp_err_t err =
      esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);

  if (err != ESP_OK) {
    HERROR("esp_ota_begin failed: %s", esp_err_to_name(err));
    // ASKA_ASSERT(false);
  }

  std::array<uint8_t, BUF_SIZE> buffer{};
  uint32_t total_received = 0;

  // we stream and write the firmware directly to the partition
  while (total_received < header.firmware_len) {
    int chunk = recv(
        client_sock, buffer.data(),
        std::min(static_cast<size_t>(BUF_SIZE),
                 static_cast<size_t>(header.firmware_len - total_received)),
        0);
    if (chunk <= 0) {
      HERROR("Firmware receive failed");
      esp_ota_abort(ota_handle);

      close(client_sock);
      close(listen_sock);
      vTaskDelete(nullptr);
      return;
    }

    err = esp_ota_write(ota_handle, buffer.data(), chunk);
    if (err != ESP_OK) {
      HERROR("esp_ota_write failed: %s", esp_err_to_name(err));
      esp_ota_abort(ota_handle);
      close(client_sock);
      close(listen_sock);
      vTaskDelete(nullptr);
      return;
    }

    total_received += chunk;

    // --- Progress bar ---
    int bar_width = 50;
    float progress = (float)total_received / header.firmware_len;
    int pos = (int)(bar_width * progress);
    screen.progress(progress);
    event_post(Event{.event_type = EventType::ScreenUpdate,
                     .screen_data = screen.data()});

    printf("\rReceiving Firmware: [");
    for (int i = 0; i < bar_width; i++) {
      if (i < pos)
        printf("=");
      else if (i == pos)
        printf(">");
      else
        printf(" ");
    }
    printf("] %3d%%", (int)(progress * 100));
    fflush(stdout);
  }
  printf("\n");

  HINFO("OTA transfer complete: %lu bytes", total_received);

  err = esp_ota_end(ota_handle);
  if (err != ESP_OK) {
    HERROR("esp_ota_end failed: %s", esp_err_to_name(err));
    close(client_sock);
    close(listen_sock);
    vTaskDelete(nullptr);
    return;
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    HERROR("esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
    close(client_sock);
    close(listen_sock);
    vTaskDelete(nullptr);
    return;
  }

  HINFO("OTA update successful. Rebooting...");
  close(client_sock);
  close(listen_sock);
  esp_restart();
}
}  // namespace hakkou