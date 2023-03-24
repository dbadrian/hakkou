#pragma once

#include "defines.h"
#include "logger.h"

#include <cstdarg>
#include <string_view>

namespace hakkou {

void platform_log(LogLevel level, std::string_view message, va_list args);

// task related
void platform_sleep(u64 ms);

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

struct PlatformConfiguration {
  bool interrupts_enabled{false};
  bool i2c_enabled{false};
  bool vfat_enabled{false};
};

bool platform_initialize(PlatformConfiguration config);

// Triggered by the esp_shutdown_handler tooling
void platform_on_shutdown(void);

using ShutdownHandler = void (*)();
bool platform_add_shutdown_handler(ShutdownHandler handler);

// misc
// TODO: wrap the timer32 here and other timer functions..?
}  // namespace hakkou
