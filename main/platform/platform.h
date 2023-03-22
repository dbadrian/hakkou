#pragma once

#include "defines.h"
#include "logger.h"

#include <cstdarg>

namespace hakkou {

void platform_log(LogLevel level,
                  const char* tag,
                  const char* format,
                  va_list args);

// task related
void platform_sleep(u64 ms);

// hardware related things
enum class GPIOPullMode { UP, DOWN, UP_DOWN };

enum class GPIOInterruptType {};

enum class GPIODirection { IN, OUT, INOUT };

struct GPIOConfig {
  uint64_t pin;
  GPIODirection direction;
  GPIOPullMode pull_mode;
  GPIOInterruptType interrupt_type;
}

void gpio_configure(const GPIOConfig& conf);
void gpio_set_direction(GPIODirection direction);
void gpio_set_pull_mode(GPIOPullMode mode);
// void gpio_wakeup_enable();

// TODO: should we define a platform_initialize
// for anything that a platofrm needs to do to ensure proper running??
// then here we can install the ISR services, configure what might be necessary for I2C etc.
// void gpio_isr_register();  // gpio_install_isr_service???
// gpio_uninstall_isr_service
// gpio_isr_handler_add

struct PlatformConfiguration {
  bool interrupt_enabled;
  bool vfat_enabled;
  u16 mhz_running;
  // ???
}

  


  // TODO: register a callback handler for a reset
  // esp_err_t esp_register_shutdown_handler(shutdown_handler_t handle);

bool platform_initialize(PlatformConfiguration config);
bool platform_on_restart(PlatformConfiguration config);
bool platform_post_restart(PlatformConfiguration config); // TODO: needed?

}  // namespace hakkou
