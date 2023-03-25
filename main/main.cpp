
// #include "defines.h"
#include "event.h"
#include "logger.h"
#include "platform/platform.h"

///////TODO: Temp includes
#include "hardware/bme280.h"
#include "hardware/fan.h"
#include "hardware/h_ds18x20.h"
#include "hardware/lcd.h"

#include "task_manager.h"
//////////

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace hakkou;

extern "C" void app_main(void) {
  // Handle any unexpected software restarts from before
  // We use this to ensure a sane state of the system
  // E.g., important GPIO to be in a known safe state.
  // TODO: handle restart

  HFATAL(" >>>>> HAKKKOU FATAL <<<<< ");
  HERROR(" >>>>> HAKKKOU ERROR <<<<< ");
  HWARN(" >>>>> HAKKKOU WARN <<<<< ");
  HINFO(" >>>>> HAKKKOU INFO <<<<< ");
  HDEBUG(" >>>>> HAKKKOU DEBUG <<<<< ");
  HTRACE(" >>>>> HAKKKOU TRACE <<<<< ");

  // First get important hardware things setup
  platform_initialize({
      .interrupts_enabled = true,
      .i2c_enabled = true,
  });

  // Initialize various subsystems
  // Note(DBA): If this were a normal application, we'd
  // also ensure to release the allocated memory etc.
  // However, most are equivalent require the whole time,
  // so memory release would be equivalent to system shutdown
  // or
  initialize_logging();
  event_initialize();

  Fan4W* fan = new Fan4W(CONFIG_FAN_PWM_PIN, CONFIG_FAN_TACHO_PIN);
  event_post(
      {.event_type = EventType::FanDuty, .sender = nullptr, .fan_duty = 0});

  LCD* lcd = new LCD();
  BME280* bme = new BME280(i2c_port_t(CONFIG_BMP280_I2C_ADDRESS),
                           static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
                           static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN), 2000);

  DS18X20* ds18x20 =
      new DS18X20(static_cast<gpio_num_t>(CONFIG_ONEWIRE_PIN), 2000);

  BaseType_t xReturned;
  TaskHandle_t xHandle = NULL;
  xTaskCreate(task_manager, "TaskManager", 2 * 2048, nullptr, 20, &xHandle);

  // Now start the main FSM
  vTaskSuspend(NULL);
}
