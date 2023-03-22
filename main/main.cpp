
// #include "defines.h"
#include "event.h"
#include "logger.h"

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

  // Initialize various subsystems
  // Note(DBA): If this were a normal application, we'd
  // also ensure to release the allocated memory etc.
  // However, most are equivalent require the whole time,
  // so memory release would be equivalent to system shutdown
  // or
  initialize_logging();
  event_initialize();
  // initializer logger if necessary
  // wifi /etc brought up by platform layer?

  // Now start the main FSM
  vTaskSuspend(NULL);
}
