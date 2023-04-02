
#include "defines.h"
#include "event.h"
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
//////////

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace hakkou;

static CallbackResponse map_ircode_to_gui_cmd(Event event, void* listener) {
  if (!event.scan_code.is_repeated) {
    switch (event.scan_code.command) {
      case IR_CMD::B_UP:
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_UP});
        break;
      case IR_CMD::B_DOWN:
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_DOWN});
        break;
      case IR_CMD::B_LEFT:
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_LEFT});
        break;
      case IR_CMD::B_RIGHT:
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_RIGHT});
        break;
      case IR_CMD::B_OK:
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_OK});
        break;
      case IR_CMD::B_ESC:
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_ESC});
        break;
      case IR_CMD::B_ONOFF:
        event_post(
            {.event_type = EventType::GUI, .gui_event = GUIEvent::GUI_ONOFF});
        break;
      default:
        break;
    }
  }
  return CallbackResponse::Continue;
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

  Fan4W* fan = new Fan4W(CONFIG_FAN_PWM_PIN, CONFIG_FAN_TACHO_PIN);
  //   event_post(
  //       {.event_type = EventType::FanDuty, .sender = nullptr, .fan_duty =
  //       0});

  //   LCD* lcd = new LCD();
  //   BME280* bme = new
  //   BME280(static_cast<i2c_port_t>(CONFIG_BMP280_I2C_ADDRESS),
  //                            static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
  //                            static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN),
  //                            2000);

  //   DS18X20* ds18x20 =
  //       new DS18X20(static_cast<gpio_num_t>(CONFIG_ONEWIRE_PIN), 2000);

  TaskHandle_t xHandle = NULL;
  xTaskCreate(task_manager, "TaskManager", 2048, nullptr, 20, &xHandle);

  //   auto heater = new Heater(CONFIG_HEATER_PWM_PIN);

  //   auto ctrl = new Controller();

  //   platform_sleep(3000);
  //   HINFO("POSTING CLD EVENT");

  //   platform_sleep(3000);
  //   HINFO("POSTING CLD EVENT");
  //   event_post(Event{
  //       .event_type = EventType::ScreenUpdate,
  //       .screen_data = {{{"                    "},
  //                        {"      > CONT. <     "},
  //                        {"   asdsd            "},
  //                        {"                    "}}},
  //   });

  //   ctrl->run(xTaskGetCurrentTaskHandle());

  auto remote = new NECRemote(CONFIG_IR_ADDR);
  event_register(EventType::IRCode, nullptr, map_ircode_to_gui_cmd);

  // Now start the main FSM
  vTaskSuspend(NULL);
}
