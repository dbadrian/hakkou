#include "fan.h"

namespace hakkou {

u16 Tachometer::rpm(u64 measurement_period) {
  u16 rpm = 0;
  ticks_.fill(0);
  // Since the frequent interrupts could conflict with other complex tasks
  // such as WIFI etc, we'd either have to pin to a specific core
  // or we just enable/disable it on request -> means we have to wait a bit
  // to obtaina  measurement
  gpio_interrupt_enable(35);
  platform_sleep(measurement_period);

  // The timer_u32 wrap around every ~53 seconds.
  // the following code will still report the correct ticks when a wrap occurs
  // due to usigned integer (Note: do not use auto or anything -> will
  // erronously upcast)
  u32 dt = ticks_.back() - ticks_.front();
  if (dt != 0) {
    rpm = FACTOR / timer_delta_us(dt);
    HDEBUG("[TACHOMETER] RPM %u", rpm);
    return rpm;
  }

  gpio_interrupt_disable(35);
  return rpm;
}

Fan4W::Fan4W(u16 pwm_pin, u16 tacho_pin)
    : pwm_pin_(pwm_pin), tacho_(tacho_pin) {
  // Register the callback to obtain and set internal fan duty
  std::optional<EventHandle> handle = event_register(
      EventType::FanDuty, static_cast<void*>(this), Fan4W::set_duty_cb);
  if (!handle) {
    HFATAL("Couldn't register fan duty callback!");
  }

  pwm_channel_ = pwm_configure({.pin = pwm_pin, .freq = PWM_FREQ});
  if (!pwm_channel_) {
    HFATAL("Couldn't configure PWM channel for Fan4W.");
  }

  // TODO: Make priority a config variable
  xTaskCreateStatic(initialize, "FAN4W", STACK_SIZE, this, PRIORITY, task_buf_,
                    &xTaskBuffer);
}

void Fan4W::run() {
  u16 rpm;

  while (true) {
    // query current RPM
    rpm = tacho_.rpm(500 /*ms*/);

    // If the rpm is below a certain threshold, we want to escalate and
    // assume something is wrong...
    HDEBUG("[Fan] RPM %lu/%i (rpm/stalling threshold)", rpm,
           RPM_STALL_THRESHOLD);

    if (rpm < RPM_STALL_THRESHOLD) {
      kill_count_++;
      if (kill_count_ >= RPM_STALL_KILL_COUNT) {
        HFATAL(
            "[FAN] Fan stalling detected!  RPM/THRESHOLD: %lu/%u. "
            "Emergency "
            "restart!",
            rpm, RPM_STALL_THRESHOLD);
        esp_restart();
      }
    } else {
      // Decrement the kill count
      if (kill_count_ > 0) {
        kill_count_ -= 1;
      }
    }

    // TODO: no error check, we dont really care right now
    event_post(Event{
        .event_type = EventType::FanRPM, .sender = nullptr, .fan_rpm = rpm});

    platform_sleep(REFRESH_MS);
  }
}

}  // namespace hakkou