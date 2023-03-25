#pragma once
#include "defines.h"
#include "logger.h"
#include "platform/platform.h"

#include "bmp280.h"

#include <cstring>

namespace hakkou {

class BME280 {
 public:
  constexpr static uint32_t STACK_SIZE = 2 * 3048;
  constexpr static UBaseType_t PRIORITY = 9;  // TODO Set priority elsewhere

  BME280(i2c_port_t address, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

 private:
  static void initialize(void* cls) { static_cast<BME280*>(cls)->run(); }

  void run();

  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;

  bmp280_params_t params_;
  bmp280_t dev_;
};

}  // namespace hakkou