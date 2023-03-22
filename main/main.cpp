
// Keep at top before first freertos import
#include "hfreertos.h"

#include "defines.h"
#include "event.h"
#include "logger.h"

using namespace hakkou;

extern "C" void app_main(void) {
  // Initialize various subsystems
  // Note(DBA): If this were a normal application, we'd
  // also ensure to release the allocated memory etc.
  // However, most are equivalent require the whole time,
  // so memory release would be equivalent to system shutdown
  // or
  initialize_logging();
  event_initialize();
  

  vTaskSuspend(nullptr);
}
