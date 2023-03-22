#include "logger.h"
// #include "asserts.h"
#include "platform/platform.h"
// // TODO: temporary
// #include <stdarg.h>
// #include <stdio.h>
// #include <string.h>

#include <cstdarg>
namespace hakkou {
bool initialize_logging() {
  // TODO: Create log files
  HINFO("Initialized Logging-subsystem");
  return true;
}

// void shutdown_logging() {
//   // TODO: ensure this is called in the esp_reset (if possible)
//   // TODO: cleanup logging/ write queued entries
// }

void log(LogLevel level, const char* message, ...) {
  // just call the platform_log for now
  va_list list;
  va_start(list, message);
  platform_log(level, "", message, list);
  va_end(list);
}

// void report_assertion_failure(const char* expression,
//                               const char* message,
//                               const char* file,
//                               i32 line) {
//   log_output(LOG_LEVEL_FATAL,
//              "Assertion Failure: %s, message: '%s', in file %s, line: %d\n",
//              expression, message, file, line);
// }

}  // namespace hakkou