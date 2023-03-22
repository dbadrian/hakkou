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

// void log(LogLevel level, const char* message, ...);

void HFATAL(std::string_view message, ...);
void HERROR(std::string_view message, ...);
void HWARN(std::string_view message, ...);
void HINFO(std::string_view message, ...);
void HDEBUG(std::string_view message, ...);
void HTRACE(std::string_view message, ...);

// // Logs a fatal-level message
// #define HFATAL(message, ...) log(LogLevel::FATAL, message, ##__VA_ARGS__);

// #ifndef HERROR
// // Logs a error-level message
// #define HERROR(message, ...) log(LogLevel::ERROR, message, ##__VA_ARGS__);
// #endif

// #if LOG_WARN_ENABLED == 1
// // Logs a error-level message
// #define HWARN(message, ...) log(LogLevel::WARN, message, ##__VA_ARGS__);
// #else
// // Does nothing when LOG_WARN_ENABLED != 1
// #define HWARN(message, ...)
// #endif

// #if LOG_INFO_ENABLED == 1
// // Logs a error-level message
// #define HINFO(message, ...) log(LogLevel::INFO, message, ##__VA_ARGS__);
// #else
// // Does nothing when LOG_INFO_ENABLED != 1
// #define HINFO(message, ...)
// #endif

// #if LOG_DEBUG_ENABLED == 1
// // Logs a error-level message
// #define HDEBUG(message, ...) log(LogLevel::DEBUG, message, ##__VA_ARGS__);
// #else
// // Does nothing when LOG_DEBUG_ENABLED != 1
// #define HDEBUG(message, ...)
// #endif

// #if LOG_TRACE_ENABLED == 1
// // Logs a error-level message
// #define HTRACE(message, ...) log(LogLevel::TRACE, message, ##__VA_ARGS__);
// #else
// // Does nothing when LOG_TRACE_ENABLED != 1
// #define HTRACE(message, ...)
// #endif

}  // namespace hakkou
