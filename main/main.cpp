
#include "defines.h"
#include "event.h"
#include "fsm.h"
#include "internal_types.h"
#include "logger.h"
#include "platform/platform.h"

///////TODO: Temp includes
#include "controller.h"
#include "hardware/bme280.h"
#include "hardware/fan.h"
#include "hardware/h_ds18x20.h"
#include "hardware/heater.h"
#include "hardware/lcd.h"

#include "hardware/nec_remote.h"

#include "task_manager.h"

#include "services/rest.h"
#include "services/wifi.h"
//////////

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace hakkou;

void setup_hardware() {
  // temporary bring up routine
  LCD* lcd = new LCD();

  auto remote = new NECRemote(CONFIG_IR_ADDR);
  if (!event_register(EventType::IRCode, nullptr, map_ircode_to_gui_cmd)) {
    HERROR("Couldn't initialize the IR remote!");
  }

  Fan4W* fan = new Fan4W(CONFIG_FAN_PWM_PIN, CONFIG_FAN_TACHO_PIN);
  event_post(
      {.event_type = EventType::FanDuty, .sender = nullptr, .fan_duty = 30});

  BME280* bme = new BME280(static_cast<i2c_port_t>(CONFIG_BMP280_I2C_ADDRESS),
                           static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
                           static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN), 2000);

  DS18X20* ds18x20 =
      new DS18X20(static_cast<gpio_num_t>(CONFIG_ONEWIRE_PIN), 2000);

  Heater* heater = new Heater(CONFIG_HEATER_PWM_PIN);
}

void setup_hardware_debug() {
  LCD* lcd = new LCD();

  // todo: create a fake food/amb temp setup
}

extern "C" void app_main(void) {
  // Handle any unexpected software restarts from before
  // We use this to ensure a sane state of the system
  // E.g., important GPIO to be in a known safe state.
  // TODO: handle restart

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
  if (!event_initialize()) {
    HERROR("Couldn't initialize the event subsystem. Halting!");
    vTaskSuspend(NULL);  // TODO: Should probably restart into error mode
  }

  // bring up wifi
  wifi_initialize();
  rest_initialize();
  setup_hardware_debug();

  TaskHandle_t xHandle = NULL;
  xTaskCreate(task_manager, "TaskManager", 2048, nullptr, 20, &xHandle);
  platform_sleep(2000);

  // auto ctrl = new Controller();
  // // wait for sensors to initialzie...
  // ctrl->run();

  MainMenuFSM* fsm = new MainMenuFSM();
  event_post(Event{
      .event_type = EventType::System,
      .system_event = SystemEventStart{},
  });

  // Now start the main FSM
  fsm->run();

  vTaskSuspend(NULL);
}
