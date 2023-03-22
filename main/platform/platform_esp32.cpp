#include "platform.h"

// #ifdef ESP_PLATFORM // TODO: This is not really checking much except that it
// was built with esp-idf...maybe enough?
#if true

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Set esp internal logging level to what matches applications
// logging level.
#if LOG_TRACE_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#elif LOG_DEBUG_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#elif LOG_WARN_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#elif LOG_INFO_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#else
#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#endif

#include <driver/gpio.h>
#include <esp_log.h>

#include <cstdio>

// as defined in esp-idf (skipping includes)

namespace hakkou {

namespace {
constexpr esp_log_level_t
    MAP_LOG_LEVELS[static_cast<std::size_t>(LogLevel::MAX_LOG_LEVELS)] = {
        ESP_LOG_ERROR,    // <- LogLevel::FATAL
        ESP_LOG_ERROR,    // <- LogLevel::ERROR
        ESP_LOG_WARN,     // <- LogLevel::WARN
        ESP_LOG_INFO,     // <- LogLevel::INFO
        ESP_LOG_DEBUG,    // <- LogLevel::DEBUG
        ESP_LOG_VERBOSE,  // <- LogLevel::TRACE
};
}

void logging_initialize_platform(LogLevel level) {
  // set logging level globally (for all tags)
  esp_log_level_set("*", MAP_LOG_LEVELS[static_cast<std::size_t>(level)]);
}

void platform_log(LogLevel level, std::string_view message, va_list args) {
  const char* colour_strings[] = {"0;41", "1;31", "1;33",
                                  "1;32", "1;34", "1;30"};
  const char* log_level_string[] = {"[FATAL]", "[ERROR]", "[WARN ]",
                                    "[INFO ]", "[DEBUG]", "[TRACE]"};
  std::printf("\033[%sm%s: ", colour_strings[static_cast<u8>(level)],
              log_level_string[static_cast<u8>(level)]);
  std::vprintf(message.data(), args);
  std::printf("\033[0m\n");
}

void platform_sleep(u64 ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

// allocation functions for freerots are also mapped to heap_caps_malloc
// so no need to us the freertos functions
// ref:
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html#freertos-heap

// https :  // youtu.be/y4xfub_s7Zk?t=2665
//  esp_log_write() esp_log_writeev()

// GPIO RELATED
void gpio_configure(const GPIOConfig& conf) {
  gpio_config_t io_conf = {};

  // convert pin number to bit mask...
  io_conf.pin_bit_mask = (u64{1} << conf.pin);

  // set input/output mode
  // TODO: Validate that the chosen pin number fits to the chosen input/output
  switch (conf.direction) {
    case GPIODirection::INPUT: {
      io_conf.mode = GPIO_MODE_INPUT;
    } break;
    case GPIODirection::OUTPUT: {
      io_conf.mode = GPIO_MODE_OUTPUT;
    } break;
    case GPIODirection::OUTPUT_OPEN_DRAIN: {
      io_conf.mode = GPIO_MODE_OUTPUT_OD;
    } break;
    case GPIODirection::INPUT_OUTPUT_OPEN_DRAIN: {
      io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    } break;
    case GPIODirection::INPUT_OUTPUT: {
      io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    } break;
    default: {
      HERROR("Invalid GPIODirection passed!");
      break;
    }
  }

  // enable pull-up
  if (conf.pull_mode == GPIOPullMode::UP ||
      conf.pull_mode == GPIOPullMode::UP_DOWN) {
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  }

  // enable pull-down
  if (conf.pull_mode == GPIOPullMode::DOWN ||
      conf.pull_mode == GPIOPullMode::UP_DOWN) {
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  }

  // set interrupt type
  switch (conf.interrupt_type) {
    case GPIOInterruptType::DISABLE: {
      io_conf.intr_type = GPIO_INTR_DISABLE;
    } break;
    case GPIOInterruptType::POSEDGE: {
      io_conf.intr_type = GPIO_INTR_POSEDGE;
    } break;
    case GPIOInterruptType::NEGEDGE: {
      io_conf.intr_type = GPIO_INTR_NEGEDGE;
    } break;
    case GPIOInterruptType::ANYEDGE: {
      io_conf.intr_type = GPIO_INTR_ANYEDGE;
    } break;
    case GPIOInterruptType::LOW_LEVEL: {
      io_conf.intr_type = GPIO_INTR_LOW_LEVEL;
    } break;
    case GPIOInterruptType::HIGH_LEVEl: {
      io_conf.intr_type = GPIO_INTR_HIGH_LEVEL;
    } break;
    default: {
      HERROR("Invalid GPIODirection passed!");
      break;
    }
  }
  gpio_config(&io_conf);

  if (conf.isr_handler != nullptr) {
    // if a handler was added, then we should have set when it should be
    // triggered!
    if (conf.interrupt_type == GPIOInterruptType::DISABLE) {
      HWARN("Added an ISR handler, but interrupt type set to disabled!");
    }
  }
  gpio_isr_handler_add(static_cast<gpio_num_t>(conf.pin), conf.isr_handler,
                       conf.isr_arg);
}

bool platform_initialize(PlatformConfiguration config) {
  if (config.interrupt_enabled) {
    switch (gpio_install_isr_service(0)) {
      case ESP_OK: {
        HINFO("Installed ISR Service");
      } break;
      case ESP_ERR_NO_MEM: {
        HINFO("Installed ISR Service");
      } break;
      case ESP_ERR_INVALID_STATE: {
        HINFO("Installed ISR Service");
      } break;
      case ESP_ERR_NOT_FOUND: {
        HINFO("Installed ISR Service");
      } break;
      case ESP_ERR_INVALID_ARG: {
        HINFO("Installed ISR Service");
      } break;
    }
  }

  // TODO: Vfat
  // TODO: ???
  return true;
}

}  // namespace hakkou
#endif