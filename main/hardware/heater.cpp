#pragma once

#include "heater.h"

namespace hakkou {

Heater::Heater(u8 pwm_pin) : pwm_pin_(pwm_pin) {
  pwm_channel_ = pwm_configure({.pin = pwm_pin_, .freq = PWM_FREQ});
  if (!pwm_channel_) {
    HFATAL("Couldn't configure PWM channel for Heater.");
  }

  std::optional<EventHandle> handle = event_register(
      EventType::HeaterDuty, static_cast<void*>(this), Heater::set_duty);
  if (!handle) {
    HFATAL("Couldn't register heater duty callback!");
  }

  HINFO("Created Heater");
}

CallbackResponse Heater::set_duty(u32 duty) {
  pwm_set_duty(pwm_channel_.value(), duty * PWM_FACTOR);
  return CallbackResponse::Stop;
}

}  // namespace hakkou