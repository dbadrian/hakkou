#include "logger.h"
// #include "asserts.h"
#include "platform.h"

// // TODO: temporary
// #include <stdarg.h>
// #include <stdio.h>
// #include <string.h>

bool initialize_logging() {
  // TODO: Create log files
  return true;
}

// void shutdown_logging() {
//   // TODO: ensure this is called in the esp_reset (if possible)
//   // TODO: cleanup logging/ write queued entries
// }

// void log_output(log_level level, const char* message, ...) {
//   const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ",
//                                   "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
//   b8 is_error = level < LOG_LEVEL_WARN;

//   // Limit of 32k characters.
//   const i32 msg_length = 32000;
//   char out_message[msg_length];
//   memset(out_message, 0, sizeof(out_message));

//   // Format the message
//   // Hint: there is some speciality here due to MS headers according
//   // to TVroman
//   __builtin_va_list arg_ptr;
//   va_start(arg_ptr, message);
//   vsnprintf(out_message, msg_length, message, arg_ptr);
//   va_end(arg_ptr);  // cleanup the va_start call

//   char out_message2[msg_length];
//   sprintf(out_message2, "%s%s\n", level_strings[level], out_message);

//   // TODO: platform-specific output.
//   // Platform-specific output.
//   if (is_error) {
//     platform_console_write_error(out_message2, level);
//   } else {
//     platform_console_write(out_message2, level);
//   }
// }

// void report_assertion_failure(const char* expression,
//                               const char* message,
//                               const char* file,
//                               i32 line) {
//   log_output(LOG_LEVEL_FATAL,
//              "Assertion Failure: %s, message: '%s', in file %s, line: %d\n",
//              expression, message, file, line);
// }
