#pragma once

#include "defines.h"
#include "event.h"
#include "logger.h"
#include "platform/platform.h"

namespace hakkou {

class Heater {
 public:
  constexpr static u8 PWM_FREQ = 20;
  constexpr static u8 PWM_LOW = 0;
  constexpr static u8 PWM_HIGH = 100;
  constexpr static auto PWM_FACTOR =
      static_cast<u32>(static_cast<float>(1 << 10) / PWM_HIGH);

  Heater(u8 pwm_pin);

  static CallbackResponse set_duty(Event event, void* listener) {
    return static_cast<Heater*>(listener)->set_duty(event.heater_duty);
  }

  CallbackResponse set_duty(u32 duty);

 private:
  u8 pwm_pin_;
  std::optional<u8> pwm_channel_;
};

}  // namespace hakkou