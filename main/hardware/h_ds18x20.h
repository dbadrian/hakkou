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
  constexpr static u32 STACK_SIZE = 2304;
  constexpr static UBaseType_t PRIORITY = 9;  // TODO Set priority elsewhere

  constexpr static std::size_t MAX_DEVICES = 10;

  DS18X20(gpio_num_t pin, u64 refresh_ms) : pin_(pin), refresh_ms_(refresh_ms) {
    assert(refresh_ms_ >= 750);

    // initial scan for sensors on the 1-wire bus
    reset();

    HINFO("Created DS18X20 Sensors-Bus");
    // TODO: Make priority a config variable
    xTaskCreateStatic(initialize, "DS18X20", STACK_SIZE, this, PRIORITY,
                      task_buf_, &xTaskBuffer);
  }

  std::size_t available_sensors() { return device_count_; }

 private:
  static void initialize(void* cls) { static_cast<DS18X20*>(cls)->run(); }

  void run() {
    // We average all attached sensors...
    // alternatively, we could send an event with the sensor id
    // and the temperature
    /// this would allow calibration outside this domain
    SlidingWindowAccumulator<float, 10> buffer;
    buffer.fill(0.0);
    float reading;

    while (true) {
      // rescan for sensors
      // TODO: we should do a comparison from now to before
      // and cause an error if sensors fail!
      reset();

      // Trigger measurement on all sensors at once
      if (ds18x20_measure(pin_, DS18X20_ANY, /*wait*/ false) != ESP_OK) {
        HERROR("DS18X20 sensor measurement failed!");
      }
      // We need to wait 750ms before reading values
      platform_sleep(750);
      // The pin is still high, since wait=False -> manually depower
      onewire_depower(pin_);

      // TODO: if device_count_ == emit system error event

      for (std::size_t i = 0; i < device_count_; ++i) {
        if (ds18x20_read_temperature(pin_, devices_[i], &reading) == ESP_OK) {
          buffer.push(reading);
        } else {
          HERROR("DS18X20 temperature reading failed: 0x%" PRIx64, devices_[i]);
        }
      }

      float mean =
          std::accumulate(buffer.begin(), buffer.end(), 0.0f) / buffer.size();

      event_post({
          .event_type = EventType::TemperatureFood,
          .sender = nullptr,
          .temperature = mean,
      });

      // TODO: MAKE CONFIGURABEL VARIABLE
      platform_sleep(refresh_ms_ - 750);
    }
    vTaskDelete(NULL);
  }

  void reset() {
    device_count_ = 0;
    if (ds18x20_scan_devices(pin_, devices_.data(), MAX_DEVICES,
                             &device_count_) != ESP_OK) {
      device_count_ = 0;
      HERROR("DS18X20 sensor scan failed!");
    }
    HDEBUG("Found %u DS18X20 sensors", device_count_);
  }

  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;

  gpio_num_t pin_;
  u64 refresh_ms_;

  std::size_t device_count_{};
  std::array<ds18x20_addr_t, MAX_DEVICES> devices_;
};

}  // namespace hakkou