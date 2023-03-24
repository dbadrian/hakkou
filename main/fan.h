#pragma once

#include "containers.h"
#include "defines.h"
#include "event.h"
#include "logger.h"

#include "ext/timer_u32.h"

#include <functional>

#define GPIO_BIT_MASK

namespace hakkou {

class Tachometer {
 public:
  // conversion factor for going from ticks-difference to RPM
  constexpr static float FACTOR = 30 * CONFIG_FAN_TACHO_WINDOW_SIZE * 1000000.0;

  Tachometer(u16 tacho_pin) : tacho_pin_(tacho_pin) {
    GPIOConfig conf = {.pin = tacho_pin,
                       .direction = GPIODirection::INPUT,
                       .pull_mode = GPIOPullMode::UP,
                       .interrupt_type = GPIOInterruptType::POSEDGE,
                       .isr_handler = callback,
                       .isr_arg = static_cast<void*>(this)};
    gpio_configure(conf);

    // disable interrupt again (despite setup)
    // will be renabled when RPM is requested on demand below
    gpio_interrupt_disable(35);
  }

  // TODO: Cleanup  the docs and the debug messaging
  u16 rpm(u64 measurement_period) {
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
  using RPM_T = u32;
  using CallbackType = std::function<void(RPM_T)>;

  constexpr static u32 STACK_SIZE = 2 * 2048;
  constexpr static UBaseType_t PRIORITY = 9;

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

  Fan4W(u16 pwm_pin, u16 tacho_pin) : pwm_pin_(pwm_pin), tacho_(tacho_pin) {
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

    // // TODO: Make priority a config variable
    xTaskCreateStatic(initialize, "FAN4W", STACK_SIZE, this, PRIORITY,
                      task_buf_, &xTaskBuffer);
  }

  static CallbackResponse set_duty_cb(Event event, void* listener) {
    return static_cast<Fan4W*>(listener)->_set_duty_impl(event.fan_duty);
  }

 private:
  static void initialize(void* cls) { static_cast<Fan4W*>(cls)->run(); }

  CallbackResponse _set_duty_impl(u32 duty) {
    HINFO("Got duty event %lu", duty);
    pwm_set_duty(pwm_channel_.value(), duty * SCALING_FACTOR);
    return CallbackResponse::Stop;
  }

  void run() {
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