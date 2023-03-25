#pragma once

#include "containers.h"
#include "defines.h"
#include "event.h"
#include "logger.h"
#include "platform/platform.h"

#include "ext/timer_u32.h"

#include <functional>

#define GPIO_BIT_MASK

namespace hakkou {

class Tachometer {
 public:
  // conversion factor for going from ticks-difference to RPM
  constexpr static float FACTOR = 30 * CONFIG_FAN_TACHO_WINDOW_SIZE * 1000000.0;

  Tachometer(u16 tacho_pin);
  // TODO: Cleanup  the docs and the debug messaging
  [[nodiscard]] u16 rpm(u64 measurement_period);

 private:
  static void callback(void* cls) {
    auto cls_ptr = static_cast<Tachometer*>(cls);
    cls_ptr->tick();
  }

  void tick() { ticks_.push(timer_u32()); }

  u16 tacho_pin_;
  SlidingWindowAccumulator<u32, CONFIG_FAN_TACHO_WINDOW_SIZE> ticks_;
};

// TODO: Document a bit more what each variable means
class Fan4W {
 public:
  constexpr static u32 STACK_SIZE = 2048;
  constexpr static UBaseType_t PRIORITY = 9;  // TODO: config!

  constexpr static int PWM_FREQ = 25000;  // 25khz, as defined by Intel standard
  constexpr static int PWM_LOW = 30;      // below 30% fan will not change
  constexpr static int PWM_HIGH = 100;    // 100%
  // compensation factor to obtain a rpm change behaviour on
  // [0...100] instead of [30...100]
  constexpr static float COMPENSATION_FACTOR =
      1.0 * (PWM_HIGH - PWM_LOW) / (100 - 0);
  // 1024 is given by the 10bit resolution of the ledc_timer
  constexpr static float PWM_FACTOR = 1024.0 / PWM_HIGH;
  constexpr static float SCALING_FACTOR = PWM_FACTOR * COMPENSATION_FACTOR;
  constexpr static int TACHO_WINDOW_SIZE = 10;

  // normal rpm in [1200, 3000]
  constexpr static uint16_t RPM_STALL_THRESHOLD = 800;
  // If N times in a row a below threshold RPM -> system reset reset
  constexpr static uint16_t RPM_STALL_KILL_COUNT = 3;

  // How many ms to sleep before sending current RPM event
  constexpr static auto REFRESH_MS = 1000;

  Fan4W(u16 pwm_pin, u16 tacho_pin);

  static CallbackResponse set_duty_cb(Event event, void* listener) {
    return static_cast<Fan4W*>(listener)->set_duty_cb(event.fan_duty);
  }

  CallbackResponse set_duty_cb(u32 duty) {
    HINFO("Got duty event %lu", duty);
    pwm_set_duty(pwm_channel_.value(), duty * SCALING_FACTOR);
    return CallbackResponse::Stop;
  }

 private:
  // Task Loop
  static void initialize(void* cls) { static_cast<Fan4W*>(cls)->run(); }
  void run();

 private:
  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;

  u16 pwm_pin_;
  Tachometer tacho_;
  std::optional<u8> pwm_channel_;

  u8 kill_count_{0};
};

}  // namespace hakkou