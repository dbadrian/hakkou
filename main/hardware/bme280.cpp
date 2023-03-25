#include "bme280.h"

#include "event.h"

namespace hakkou {

BME280::BME280(i2c_port_t address, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
  // Following tutorial:
  // github.com/UncleRus/esp-idf-lib/blob/master/examples/bmp280/main/main.c

  bmp280_init_default_params(&params_);
  memset(&dev_, 0, sizeof(bmp280_t));

  // TODO: Replace error check with custom logic
  // that halts the system, but also displays error accordingly
  ESP_ERROR_CHECK(
      bmp280_init_desc(&dev_, address, i2c_port_t(0), sda_gpio, scl_gpio));
  ESP_ERROR_CHECK(bmp280_init(&dev_, &params_));

  bool bme280p = dev_.id == BME280_CHIP_ID;
  HINFO("BMP280: found %s", bme280p ? "BME280" : "BMP280");
  // TODO: Make priority a config variable
  xTaskCreateStatic(initialize, "BME280", STACK_SIZE, this, PRIORITY, task_buf_,
                    &xTaskBuffer);
}

void BME280::run() {
  float pressure = 0.0, temperature = 0.0, humidity = 0.0;

  while (true) {
    if (bmp280_read_float(&dev_, &temperature, &pressure, &humidity) !=
        ESP_OK) {
      // TODO: Read error really should only occur if the sensor was
      // disconnected As a consequence -> restart and lock system
      printf("Temperature/pressure reading failed\n");
      platform_sleep(10000);
      continue;
    }

    HINFO("HMD: %f, TMP: %f, PRS: %f", humidity, temperature, pressure);
    event_post(
        {.event_type = EventType::HumidityAmbient, .humidity = humidity});
    event_post({.event_type = EventType::TemperatureAmbient,
                .temperature = temperature});
    platform_sleep(1000);
  }
  vTaskDelete(NULL);
}

}  // namespace hakkou