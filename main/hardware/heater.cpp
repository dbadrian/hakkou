#pragma once

#include "heater.h"

namespace hakkou {

Heater::Heater(u8 pwm_pin) : pwm_pin_(pwm_pin) {
  pwm_channel_ = pwm_configure({.pin = pwm_pin_, .freq = PWM_FREQ});
  if (!pwm_channel_) {
    HFATAL("Couldn't configure PWM channel for Heater.");
  }

  HINFO("Created Heater");
}

void Heater::set_duty(u32 duty) {
  pwm_set_duty(pwm_channel_.value(), duty * PWM_FACTOR);
}

}  // namespace hakkou