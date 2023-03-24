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

#include <esp_log.h>
#include "driver/gpio.h"
#include "i2cdev.h"
#include "nvs_flash.h"

#include <cstdio>

// as defined in esp-idf (skipping includes)

namespace hakkou {

namespace {
constexpr esp_log_level_t
    // Convert internal log-levels to the esp-idf log levels
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

namespace {
using D = hakkou::GPIODirection;
// Set entry to the minimum level this GPIO (position == gpio_num)
// supports.
// Input is the lowest (1), hence 0 indicates that GPIO number can't be
// used/doesnt exist.
constexpr u16 GPIO_VALID_MODE[GPIO_NUM_MAX] = {
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO0, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO1, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO2, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO3, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO4, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO5, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO6, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO7, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO8, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO9, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO10, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO11, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO12, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO13, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO14, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO15, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO16, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO17, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO18, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO19, input and output */
    0,                                 /* NA */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO21, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO22, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO23, input and output */
    0,                                 /* NA */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO25, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO26, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO27, input and output */
    0,                                 /* NA */
    0,                                 /* NA */
    0,                                 /* NA */
    0,                                 /* NA */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO32, input and output */
    static_cast<u16>(D::INPUT_OUTPUT), /*!< GPIO32, input and output */
    static_cast<u16>(D::INPUT),        /*!< GPIO34, input mode only */
    static_cast<u16>(D::INPUT),        /*!< GPIO35, input mode only */
    static_cast<u16>(D::INPUT),        /*!< GPIO36, input mode only */
    static_cast<u16>(D::INPUT),        /*!< GPIO37, input mode only */
    static_cast<u16>(D::INPUT),        /*!< GPIO38, input mode only */
    static_cast<u16>(D::INPUT),        /*!< GPIO39, input mode only */
};

}  // namespace

// GPIO RELATED
bool gpio_configure(const GPIOConfig& conf) {
  gpio_config_t io_conf = {};

  // convert pin number to bit mask...
  io_conf.pin_bit_mask = (u64{1} << conf.pin);

  // validate that the chosen pin number fits to the chosen input/output
  if (static_cast<u16>(conf.direction) > GPIO_VALID_MODE[conf.pin]) {
    // Not really necessary...esp-idf will also report error, but this
    // way we can also log it elsewhere...
    HFATAL("Invalid GPIO number given!");
    return false;
  }

  // set input/output mode
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
      return false;
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
  switch (gpio_config(&io_conf)) {
    case ESP_OK: {
    } break;
    case ESP_ERR_INVALID_ARG: {
      HERROR("Parameter error when configuring GPIO pin %ul!", conf.pin);
      return false;
    }
  }

  // Add any request ISR handlers after verifying we can actually do so
  if (conf.isr_handler != nullptr) {
    // if a handler was added, then we should have set when it should be
    // triggered on!
    if (conf.interrupt_type == GPIOInterruptType::DISABLE) {
      HERROR("Request ISR handler, but interrupt type set to disabled!");
      return false;
    }
  }
  esp_err_t isr_status_ret = gpio_isr_handler_add(
      static_cast<gpio_num_t>(conf.pin), conf.isr_handler, conf.isr_arg);
  switch (isr_status_ret) {
    case ESP_OK: {
      HINFO("Added ISR handler");
    } break;
    case ESP_ERR_INVALID_STATE: {
      HERROR("Wrong state, the ISR service has not been initialized.");
      return false;
    }
    case ESP_ERR_INVALID_ARG: {
      HERROR("Parameter error ");
      return false;
    }
  }

  // everything configure correctly...
  return true;
}

bool gpio_interrupt_enable(u16 pin) {
  // TODO: handle errors in gpio interrupt dis/enable correctly
  ESP_ERROR_CHECK(gpio_intr_enable(static_cast<gpio_num_t>(pin)));
  return true;
}
bool gpio_interrupt_disable(u16 pin) {
  // TODO: handle errors in gpio interrupt dis/enable correctly
  ESP_ERROR_CHECK(gpio_intr_disable(static_cast<gpio_num_t>(pin)));
  return true;
}

bool platform_initialize(PlatformConfiguration config) {
  if (!platform_add_shutdown_handler(&platform_on_shutdown)) {
    HERROR(
        "Couldn't install the system-wide platform shutdown handler. Aborting "
        "for safety.");
    return false;
  }

  if (config.interrupts_enabled) {
    switch (gpio_install_isr_service(0)) {
      case ESP_OK: {
        HINFO("Installed ISR Service.");
      } break;
      case ESP_ERR_NO_MEM: {
        HERROR("No memory to install this service.");
        return false;
      }
      case ESP_ERR_INVALID_STATE: {
        HERROR("ISR service already installed.");
        return false;
      }
      case ESP_ERR_NOT_FOUND: {
        HERROR("No free interrupt found with the specified flags.");
        return false;
      }
      case ESP_ERR_INVALID_ARG: {
        HERROR("GPIO error.");
        return false;
      }
    }
  }

  // TODO:
  // Internal flash storage
  // ESP_ERROR_CHECK(nvs_flash_init());
  // ESP_ERROR_CHECK(init_fs());

  if (config.i2c_enabled) {
    switch (i2cdev_init()) {
      case ESP_OK: {
        HINFO("Initialized I2C.");
      } break;
      default: {
        HERROR("Unknown error occurred initializing I2C.");
        return false;
      }
    }
  }

  return true;
}

bool platform_add_shutdown_handler(ShutdownHandler handler) {
  switch (esp_register_shutdown_handler(handler)) {
    case ESP_OK: {
      HINFO("Registered Platform Shutdown Handler.");
    } break;
    case ESP_ERR_INVALID_STATE: {
      HERROR("Handler already registered");
      return false;
    }
    case ESP_ERR_NO_MEM: {
      HERROR("No more shutdown handler slots are available");
      return false;
    }
  }

  return true;
}

void platform_on_shutdown(void) {
  // TODO: Log more? FIlesystem shutdown? dunnno
  HWARN("System shutdown occured....");
}

}  // namespace hakkou
#endif