#pragma once
#include "defines.h"
#include "logger.h"
#include "platform/platform.h"

#include "bmp280.h"

#include <cstring>

namespace hakkou {

class BME280 {
 public:
  constexpr static uint32_t STACK_SIZE = 2304;
  constexpr static UBaseType_t PRIORITY = 9;  // TODO Set priority elsewhere

  BME280(i2c_port_t address,
         gpio_num_t sda_gpio,
         gpio_num_t scl_gpio,
         u64 refresh_ms);

 private:
  static void initialize(void* cls) { static_cast<BME280*>(cls)->run(); }

  void run();

  // how often to measure and send update
  u64 refresh_ms_;

  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;

  bmp280_params_t params_;
  bmp280_t dev_;
};

}  // namespace hakkou