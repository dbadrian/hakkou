#pragma once
#include "defines.h"
#include "logger.h"
#include "platform/platform.h"

#include "sht3x.h"

#include <cstring>

namespace hakkou {

class SHT31D {
 public:
  constexpr static uint32_t STACK_SIZE = 4192;
  constexpr static UBaseType_t PRIORITY = 9;  // TODO Set priority elsewhere

  SHT31D(i2c_port_t address,
         gpio_num_t sda_gpio,
         gpio_num_t scl_gpio,
         u64 refresh_ms) {
    memset(&dev, 0, sizeof(sht3x_t));
    ESP_ERROR_CHECK(sht3x_init_desc(&dev, address, i2c_port_t(0), sda_gpio, scl_gpio));
    ESP_ERROR_CHECK(sht3x_init(&dev));
  xTaskCreateStatic(initialize, "SHT31D", STACK_SIZE, this, PRIORITY, task_buf_,
                    &xTaskBuffer);
  }

 private:
  static void initialize(void* cls) { static_cast<SHT31D*>(cls)->run(); }

  void run() {
    float temperature = 0.0, humidity = 0.0;

  while (true) {
    if (sht3x_measure(&dev, &temperature, &humidity) !=
        ESP_OK) {
      // TODO: Read error really should only occur if the sensor was
      // disconnected As a consequence -> restart and lock system
      printf("Temperature/pressure reading failed\n");
      platform_sleep(10000);
      continue;
    }

    HINFO("HMD: %f, TMP: %f, PRS: %f", humidity, temperature, 0);
    event_post(
        {.event_type = EventType::HumidityAmbient, .humidity = humidity});
    event_post({.event_type = EventType::TemperatureAmbient,
                .temperature = temperature});
    platform_sleep(1000);
  }
  vTaskDelete(NULL);
}


  // how often to measure and send update
  u64 refresh_ms_;

  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;

  sht3x_t dev;
};

}  // namespace hakkou