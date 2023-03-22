#pragma once

#include "defines.h"

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
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5,
  //
  MAX_LOG_LEVELS
};

}

bool initialize_logging();
void shutdown_logging();

// void log_output(log_level level, const char* message, ...);

// Logs a fatal-level message
#define KFATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__)

#ifndef KERROR
// Logs a error-level message
#define KERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#endif

#if LOG_WARN_ENABLED == 1
// Logs a error-level message
#define KWARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__)
#else
// Does nothing when LOG_WARN_ENABLED != 1
#define KWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
// Logs a error-level message
#define KINFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define KINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a error-level message
#define KDEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#define KDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// Logs a error-level message
#define KTRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__)
#else
// Does nothing when LOG_TRACE_ENABLED != 1
#define KTRACE(message, ...)
#endif
