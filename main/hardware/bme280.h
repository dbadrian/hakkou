#pragma once
#include "defines.h"
#include "logger.h"

#include "bmp280.h"

#include "freertos/semphr.h"

#include <cstring>

namespace hakkou {

const static char* BME280_TAG = "BME280";
class BME280 {
 public:
  using TemperatureT = float;
  using HumidityT = float;

  using TemperatureCallbackType = std::function<void(TemperatureT)>;
  using HumidityCallbackType = std::function<void(HumidityT)>;

  constexpr static uint32_t STACK_SIZE = 2 * 2048;
  constexpr static UBaseType_t PRIORITY = 9;

  BME280(i2c_port_t address,
         gpio_num_t sda_gpio,
         gpio_num_t scl_gpio,
         TemperatureCallbackType temp_cb,
         HumidityCallbackType hmd_cb)
      : temp_cb_(temp_cb), hmd_cb_(hmd_cb) {
    // Following tutorial:
    //
    // github.com/UncleRus/esp-idf-lib/blob/master/examples/bmp280/main/main.c

    bmp280_init_default_params(&params_);
    memset(&dev_, 0, sizeof(bmp280_t));

    ESP_ERROR_CHECK(
        bmp280_init_desc(&dev_, address, i2c_port_t(0), sda_gpio, scl_gpio));
    ESP_ERROR_CHECK(bmp280_init(&dev_, &params_));

    bme280p = dev_.id == BME280_CHIP_ID;
    ESP_LOGI(BME280_TAG, "BMP280: found %s", bme280p ? "BME280" : "BMP280");

    ESP_LOGI(BME280_TAG, "Created BME280");
    // TODO: Make priority a config variable
    xTaskCreateStatic(initialize, "BME280", STACK_SIZE, this, PRIORITY,
                      task_buf_, &xTaskBuffer);
  }

 private:
  static void initialize(void* cls) { static_cast<BME280*>(cls)->run(); }

  void run() {
    constexpr static std::size_t WINDOW_SIZE = 5;
    SlidingWindowAccumulator<float, WINDOW_SIZE> humidity_avg, temperature_avg;
    float pressure = 0.0, temperature = 0.0, humidity = 0.0;

    while (true) {
      vTaskDelay(500_ms2t);
      if (bmp280_read_float(&dev_, &temperature, &pressure, &humidity) !=
          ESP_OK) {
        // TODO: handle error state
        printf("Temperature/pressure reading failed\n");
        vTaskDelay(10000_ms2t);
        continue;
      }

      humidity = humidity_avg.update_and_accumulate(humidity);
      temperature = temperature_avg.update_and_accumulate(temperature);

      if (hmd_cb_) {
        hmd_cb_(humidity);
      }

      if (temp_cb_) {
        temp_cb_(temperature);
      }
    }
    vTaskDelete(NULL);
  }

  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;

  TemperatureCallbackType temp_cb_;
  HumidityCallbackType hmd_cb_;

  bool bme280p;
  bmp280_params_t params_;
  bmp280_t dev_;
};

}  // namespace hakkou