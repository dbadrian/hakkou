
#include "defines.h"
#include "event.h"
#include "fsm.h"
#include "internal_types.h"
#include "logger.h"
#include "platform/platform.h"

///////TODO: Temp includes
#include "controller.h"
// #include "hardware/bme280.h"
#include "hardware/fan.h"
#include "hardware/h_ds18x20.h"
#include "hardware/heater.h"
#include "hardware/lcd.h"
#include "hardware/sht31d.h"

#include <encoder.h>

#include "soc/gpio_num.h"
#include "task_manager.h"

#include "services/rest.h"
#include "services/wifi.h"
//////////

#include "esp_log.h"

#include <rom/ets_sys.h>  // us delay
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace hakkou;

void rotary_encoder_task(void* arg) {
#define RE_A_GPIO gpio_num_t(19)
#define RE_B_GPIO gpio_num_t(23)
#define RE_BTN_GPIO gpio_num_t(18)
#define EV_QUEUE_LEN 5

  QueueHandle_t event_queue;
  rotary_encoder_t re;

  // Create event queue for rotary encoders
  event_queue = xQueueCreate(EV_QUEUE_LEN, sizeof(rotary_encoder_event_t));

  // Setup rotary encoder library
  ESP_ERROR_CHECK(rotary_encoder_init(event_queue));

  // Add one encoder
  memset(&re, 0, sizeof(rotary_encoder_t));
  re.pin_a = RE_A_GPIO;
  re.pin_b = RE_B_GPIO;
  re.pin_btn = RE_BTN_GPIO;
  ESP_ERROR_CHECK(rotary_encoder_add(&re));

  rotary_encoder_event_t e;
  int32_t val = 0;

  HINFO("Initial value: %" PRIi32, val);
  while (1) {
    xQueueReceive(event_queue, &e, portMAX_DELAY);

    switch (e.type) {
      case RE_ET_BTN_PRESSED:
        HINFO("Button pressed");
        event_post({.event_type = EventType::RotaryEncoder, .sender=nullptr, .rotary_event=RotaryEncoderEvent::BUTTON_PRESSED});
        break;
      case RE_ET_BTN_RELEASED:
        HINFO("Button released");
        event_post({.event_type = EventType::RotaryEncoder, .sender=nullptr, .rotary_event=RotaryEncoderEvent::BUTTON_RELEASED});
        break;
      case RE_ET_BTN_CLICKED:
        HINFO("Button clicked");
        // rotary_encoder_enable_acceleration(&re, 100);
        event_post({.event_type = EventType::RotaryEncoder, .sender=nullptr, .rotary_event=RotaryEncoderEvent::BUTTON_CLICKED});
        break;
      case RE_ET_BTN_LONG_PRESSED:
        HINFO("Looooong pressed button");
        // rotary_encoder_disable_acceleration(&re);
        event_post({.event_type = EventType::RotaryEncoder, .sender=nullptr, .rotary_event=RotaryEncoderEvent::BUTTON_LONG_PRESS});
        break;
      case RE_ET_CHANGED:
        if (e.diff > 0) {
                  event_post({.event_type = EventType::RotaryEncoder, .sender=nullptr, .rotary_event=RotaryEncoderEvent::LEFT});

        } else {
          event_post({.event_type = EventType::RotaryEncoder, .sender=nullptr, .rotary_event=RotaryEncoderEvent::RIGHT});
        }
        // val += e.diff;
        // HINFO("Value = %" PRIi32, val);
        break;
      default:
        break;
    }
  }
}

void setup_hardware() {
  // temporary bring up routine
  LCD* lcd = new LCD();

  // auto remote = new NECRemote(CONFIG_IR_ADDR);
  // if (!event_register(EventType::IRCode, nullptr, map_ircode_to_gui_cmd)) {
  //   HERROR("Couldn't initialize the IR remote!");
  // }

  // Fan4W* fan = new Fan4W(CONFIG_FAN_PWM_PIN, CONFIG_FAN_TACHO_PIN);
  // event_post(
  //     {.event_type = EventType::FanDuty, .sender = nullptr, .fan_duty = 30});

  // // BME280* bme = new
  // // BME280(static_cast<i2c_port_t>(CONFIG_BMP280_I2C_ADDRESS),
  // //                          static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
  // //                          static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN),
  // //                          2000);

  // SHT31D* whatever =
  //     new SHT31D(static_cast<i2c_port_t>(0x44),
  //                static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
  //                static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN), 2000);

  // DS18X20* ds18x20 =
  //     new DS18X20(static_cast<gpio_num_t>(CONFIG_ONEWIRE_PIN), 2000);

  // Heater* heater = new Heater(CONFIG_HEATER_PWM_PIN);
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

  gpio_configure({
      .pin = 4,
      .direction = GPIODirection::OUTPUT,
      .pull_mode = GPIOPullMode::UP,
  });

  xTaskCreate(rotary_encoder_task, "RotaryEncoder",
              configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);

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
  setup_hardware();

  TaskHandle_t xHandle = NULL;
  xTaskCreate(task_manager, "TaskManager", 4192, nullptr, 20, &xHandle);
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
