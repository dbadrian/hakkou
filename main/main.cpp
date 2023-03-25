
// #include "defines.h"
#include "event.h"
#include "logger.h"
#include "platform/platform.h"

///////TODO: Temp includes
#include "hardware/fan.h"
#include "hardware/lcd.h"
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

  Fan4W* fan = new Fan4W(32, 35);
  event_post(
      {.event_type = EventType::FanDuty, .sender = nullptr, .fan_duty = 0});

  LCD* lcd = new LCD();
  platform_sleep(1000);
  HINFO("Trying to send message to lcd");
  static char screen_buf[4][20 + 1] = {{" Press ESC to abort "},
                                       {" Press ESC to abort "},
                                       {" Press ESC to abort "},
                                       {" Press OK to cont.  "}};
  lcd->update(screen_buf[0]);
  platform_sleep(5000);
  static char screen_bu2f[4][20 + 1] = {{" Press ESC to abort "},
                                        {" Press ABC to abort "},
                                        {" Press ESC to abort "},
                                        {" Press KO to cont.  "}};
  lcd->update(screen_bu2f[0]);
  // initializer logger if necessary
  // wifi /etc brought up by platform layer?

  // Now start the main FSM
  vTaskSuspend(NULL);
}
