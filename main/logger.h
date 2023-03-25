#pragma once

#include "defines.h"

#include <string_view>

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#if HRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

namespace hakkou {

enum class LogLevel {
  FATAL = 0,
  ERROR,
  WARN,
  INFO,
  DEBUG,
  TRACE,
  //
  MAX_LOG_LEVELS
};

bool initialize_logging();
void shutdown_logging();

void HFATAL(std::string_view message, ...);
void HERROR(std::string_view message, ...);
void HWARN(std::string_view message, ...);
void HINFO(std::string_view message, ...);
void HDEBUG(std::string_view message, ...);
void HTRACE(std::string_view message, ...);

}  // namespace hakkou
