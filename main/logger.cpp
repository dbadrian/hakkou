#include "logger.h"
// #include "asserts.h"
#include "platform/platform.h"

#include <cstdarg>
#include <string_view>

namespace hakkou {
bool initialize_logging() {
  // TODO: Create log files
  HINFO("Initialized Logging-subsystem");
  return true;
}

void shutdown_logging() {
  // TODO: ensure this is called in the esp_reset (if possible)
  // TODO: cleanup logging/ write queued entries
}

void HFATAL(std::string_view message, ...) {
  va_list list;
  va_start(list, message);
  platform_log(LogLevel::FATAL, message, list);
  va_end(list);
}

void HERROR(std::string_view message, ...) {
  va_list list;
  va_start(list, message);
  platform_log(LogLevel::ERROR, message, list);
  va_end(list);
}

void HWARN(std::string_view message, ...) {
#if LOG_WARN_ENABLED == 1
  va_list list;
  va_start(list, message);
  platform_log(LogLevel::WARN, message, list);
  va_end(list);
#else
// Does nothing when LOG_WARN_ENABLED != 1
#endif
}

void HINFO(std::string_view message, ...) {
#if LOG_INFO_ENABLED == 1
  va_list list;
  va_start(list, message);
  platform_log(LogLevel::INFO, message, list);
  va_end(list);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#endif
}

void HDEBUG(std::string_view message, ...) {
#if LOG_DEBUG_ENABLED == 1
  va_list list;
  va_start(list, message);
  platform_log(LogLevel::DEBUG, message, list);
  va_end(list);
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#endif
}

void HTRACE(std::string_view message, ...) {
#if LOG_TRACE_ENABLED == 1
  va_list list;
  va_start(list, message);
  platform_log(LogLevel::TRACE, message, list);
  va_end(list);
#else
// Does nothing when LOG_TRACE_ENABLED != 1
#endif
}

}  // namespace hakkou