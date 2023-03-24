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
  u32 rpm(u64 measurement_period) {
    u32 rpm = 0;
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
      // HDEBUG("[TACHOMETER] %u; %f; %lf; %lu", rpm, FACTOR,
      // timer_delta_us(dt),
      //        dt);
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

// // TODO: move to designated initializer in C++20
// class Fan4W {
//  public:
//   using RPM_T = u32;
//   using CallbackType = std::function<void(RPM_T)>;

//   constexpr static u32 STACK_SIZE = 2 * 2048;
//   constexpr static UBaseType_t PRIORITY = 9;

//   constexpr static int PWM_FREQ = 25000;  // 25khz, as defined by Intel
//   standard constexpr static int PWM_LOW = 30;      // below 30% fan will not
//   change constexpr static int PWM_HIGH = 100;    // 100%
//   // compensation factor to obtain a rpm change behaviour on
//   // [0...100] instead of [30...100]
//   constexpr static float COMPENSATION_FACTOR =
//       1.0 * (PWM_HIGH - PWM_LOW) / (100 - 0);
//   // 1024 is given by the 10bit resolution of the ledc_timer
//   constexpr static float PWM_FACTOR = 1024.0 / PWM_HIGH;
//   constexpr static float SCALING_FACTOR = PWM_FACTOR * COMPENSATION_FACTOR;
//   constexpr static int TACHO_WINDOW_SIZE = 10;

//   constexpr static uint16_t RPM_STALL_THRESHOLD =
//       800;  // normal rpm in [1200, 2800]
//   // If N times in a row, we measure below threshold RPM we perform a system
//   // reset
//   constexpr static uint16_t RPM_STALL_KILL_COUNT =
//       3;  // normal rpm in [1200, 2800]

//   constexpr static auto REFRESH_MS = 1000 / portTICK_PERIOD_MS;

//   Fan4W(int pwm_pin, gpio_num_t tacho_pin, CallbackType rpm_callback)
//       : pwm_pin_(pwm_pin), tacho_(tacho_pin), rpm_callback_(rpm_callback) {
//     ledc_timer_config_t ledc_timer = {
//         .speed_mode = LEDC_LOW_SPEED_MODE,
//         .duty_resolution = LEDC_TIMER_10_BIT,
//         .timer_num = LEDC_TIMER_0,
//         .freq_hz = PWM_FREQ,  // Set output frequency at 5 kHz
//         .clk_cfg = LEDC_AUTO_CLK,
//     };

//     // auto fn = [this](const Broker::MessageType &msg) {
//     this->set_duty(msg);
//     // }; broker.subscribe(BrokerTopic::FAN_DUTY, fn);

//     ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

//     // Prepare and then apply the LEDC PWM channel configuration
//     ledc_channel_config_t ledc_channel = {
//         .gpio_num = pwm_pin,
//         .speed_mode = LEDC_LOW_SPEED_MODE,
//         .channel = static_cast<ledc_channel_t>(CONFIG_FAN_PWM_CHANNEL),
//         .intr_type = LEDC_INTR_DISABLE,
//         .timer_sel = LEDC_TIMER_0,
//         .duty = 0,  // Set duty to 0%
//         .hpoint = 0};
//     ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

//     ESP_LOGD(FAN_TAG, "Created FAN4W");
//     // TODO: Make priority a config variable
//     xTaskCreateStatic(initialize, "FAN4W", STACK_SIZE, this, PRIORITY,
//                       task_buf_, &xTaskBuffer);
//   }

//   void set_duty(u32 duty) {
//     auto duty_ = static_cast<u32>(duty * SCALING_FACTOR);
//     ESP_LOGD(FAN_TAG, "SETTINING DUTY %lu", duty);
//     ESP_ERROR_CHECK(ledc_set_duty(
//         LEDC_LOW_SPEED_MODE,
//         static_cast<ledc_channel_t>(CONFIG_FAN_PWM_CHANNEL), duty_));
//     // Update duty to apply the new value
//     ESP_ERROR_CHECK(
//         ledc_update_duty(LEDC_LOW_SPEED_MODE,
//                          static_cast<ledc_channel_t>(CONFIG_FAN_PWM_CHANNEL)));
//   }

//  private:
//   static void initialize(void* cls) { static_cast<Fan4W*>(cls)->run(); }

//   void run() {
//     u32 rpm;
//     // boot up: allow the sliding window average to fill with values
//     vTaskDelay(500);
//     while (true) {
//       rpm = tacho_.rpm();

//       // If the rpm is below a certain threshold, we want to escalate and
//       assume
//       // something is wrong...
//       ESP_LOGD("FAN", "Fan rpm %lu/%i", rpm, RPM_STALL_THRESHOLD);
//       if (rpm < RPM_STALL_THRESHOLD) {
//         kill_count_++;
//         if (kill_count_ >= RPM_STALL_KILL_COUNT) {
//           ESP_LOGE(FAN_TAG,
//                    "Dropped below RPM STALLING THRESHOLD: %lu/%u. Emergency "
//                    "restart!",
//                    rpm, RPM_STALL_THRESHOLD);
//           esp_restart();
//         }
//       } else {
//         // Reset the kill count
//         kill_count_ = 0;
//       }

//       if (rpm_callback_) {
//         rpm_callback_(rpm);
//       }

//       vTaskDelay(REFRESH_MS);
//     }
//   }

//  private:
//   // Task/Stack buffer
//   StackType_t task_buf_[STACK_SIZE];
//   StaticTask_t xTaskBuffer;

//   int pwm_pin_;
//   Tachometer<TACHO_WINDOW_SIZE> tacho_;
//   CallbackType rpm_callback_;

//   uint8_t kill_count_{0};
// };

}  // namespace hakkou