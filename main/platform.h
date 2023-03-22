#pragma once

#include "defines.h"
#include "logger.h"

#include <cstdarg>

namespace hakkou {

void platform_log(LogLevel level,
                  const char* tag,
                  const char* format,
                  va_list args);

void platform_sleep(u64 ms);

}  // namespace hakkou
