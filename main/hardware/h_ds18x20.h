#pragma once

#include "containers.h"
#include "defines.h"
#include "event.h"
#include "logger.h"
#include "platform/platform.h"

#include <onewire.h>
#include "ds18x20.h"

#include <inttypes.h>
#include <array>
#include <numeric>

namespace hakkou {

class DS18X20 {
 public:
  constexpr static u32 STACK_SIZE = 4192;
  constexpr static UBaseType_t PRIORITY = 9;  // TODO Set priority elsewhere

  constexpr static std::size_t MAX_DEVICES = 10;

  DS18X20(gpio_num_t pin, u64 refresh_ms);

  std::size_t available_sensors() { return device_count_; }

 private:
  static void initialize(void* cls) { static_cast<DS18X20*>(cls)->run(); }

  void run();
  void reset();

  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;

  gpio_num_t pin_;
  u64 refresh_ms_;

  std::size_t device_count_{};
  std::array<ds18x20_addr_t, MAX_DEVICES> devices_;
};

}  // namespace hakkou