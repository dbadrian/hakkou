#pragma once

#include "defines.h"
#include "logger.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <cstdarg>
#include <optional>
#include <string_view>




#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>           // for close()
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <esp_vfs.h>
#include <esp_vfs_dev.h>
#include <esp_log_internal.h>

static int client_sock = -1;

static int platform_vprintf(const char* fmt, va_list args) {
  char buf[256];
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  if (client_sock >= 0) {
    send(client_sock, buf, len, 0);
  }
  return vprintf(fmt, args);
}

static void platform_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    platform_vprintf(format, args);
    va_end(args);
}

#define PORT 9000

static void log_server_task(void* arg) {
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);

  int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(listen_sock, 1);

  while (1) {
    client_sock =
        accept(listen_sock, (struct sockaddr*)&client_addr, &addr_len);
    // ESP_LOGI(TAG, "Client connected");

    while (1) {
      char tmp[32];
      int len = recv(client_sock, tmp, sizeof(tmp), 0);
      if (len <= 0)
        break;
    }

    // ESP_LOGI(TAG, "Client disconnected");
    close(client_sock);
    client_sock = -1;
  }
}



namespace hakkou {

void platform_log(LogLevel level, std::string_view message, va_list args);

// task / thread related
void platform_sleep(u64 ms);

class HMutex {
 public:
  HMutex();
  ~HMutex();

  // Blocks until a lock can be acquired for the current execution agent
  // (thread, process, task). If an exception is thrown, no lock is acquired.
  void lock();

  [[nodiscard]] bool lock(u64 ms_to_wait);

  [[nodiscard]] bool try_lock();

  void unlock();

 private:
  // Not very platform independent, but freertos is basis anyway
  // and pimpl would be not very performance-nice
  SemaphoreHandle_t xSemaphore_{nullptr};
  StaticSemaphore_t xMutexBuffer_;
};

// hardware related things
enum class GPIOPullMode { UP, DOWN, UP_DOWN };

enum class GPIOInterruptType {
  DISABLE,
  POSEDGE,
  NEGEDGE,
  ANYEDGE,
  LOW_LEVEL,
  HIGH_LEVEl,
};

enum class GPIODirection {
  INPUT = 1,
  OUTPUT = 2,
  OUTPUT_OPEN_DRAIN = 3,
  INPUT_OUTPUT_OPEN_DRAIN = 4,
  INPUT_OUTPUT = 5
};

// ISR callback signature
using ISRHandler = void (*)(void* arg);

struct GPIOConfig {
  u16 pin;
  GPIODirection direction;
  GPIOPullMode pull_mode;
  GPIOInterruptType interrupt_type;
  ISRHandler isr_handler;
  void* isr_arg;
};

bool gpio_configure(const GPIOConfig& conf);
// void gpio_set_direction(GPIODirection direction);
// void gpio_set_pull_mode(GPIOPullMode mode);
// void gpio_wakeup_enable();

bool gpio_interrupt_enable(u16 pin);
bool gpio_interrupt_disable(u16 pin);

struct PWMConfig {
  u16 pin;
  u32 freq;
};

std::optional<u8> pwm_configure(const PWMConfig& conf);
bool pwm_set_duty(u8 channel_id, u32 duty);

struct PlatformConfiguration {
  bool interrupts_enabled{false};
  bool i2c_enabled{false};
  bool vfat_enabled{false};
  bool nvs_enabled{false};
};

bool platform_initialize(PlatformConfiguration config);

// Triggered by the esp_shutdown_handler tooling
void platform_on_shutdown(void);

using ShutdownHandler = void (*)();
bool platform_add_shutdown_handler(ShutdownHandler handler);

[[nodiscard]] u32 get_time_sec();

// misc
// TODO: wrap the timer32 here and other timer functions..?
}  // namespace hakkou
