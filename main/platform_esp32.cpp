#include "platform.h"

// #ifdef ESP_PLATFORM // TODO: This is not really checking much except that it
// was built with esp-idf...maybe enough?
#if true


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Set esp internal logging level to what matches applications
// logging level.
#if LOG_TRACE_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#elif LOG_DEBUG_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#elif LOG_WARN_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_WARN
#elif LOG_INFO_ENABLED
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#else
#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#endif

#include <esp_log.h>

#include <cstdio>


// as defined in esp-idf (skipping includes)

namespace hakkou {

namespace {
constexpr esp_log_level_t
    MAP_LOG_LEVELS[static_cast<std::size_t>(LogLevel::MAX_LOG_LEVELS)] = {
        ESP_LOG_ERROR,    // <- LogLevel::FATAL
        ESP_LOG_ERROR,    // <- LogLevel::ERROR
        ESP_LOG_WARN,     // <- LogLevel::WARN
        ESP_LOG_INFO,     // <- LogLevel::INFO
        ESP_LOG_DEBUG,    // <- LogLevel::DEBUG
        ESP_LOG_VERBOSE,  // <- LogLevel::TRACE
};
}

void logging_initialize_platform(LogLevel level) {
  // set logging level globally (for all tags)
  esp_log_level_set("*", MAP_LOG_LEVELS[static_cast<std::size_t>(level)]);
}

void platform_console_write(const char* message, u8 colour) {
  // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
  const char* colour_strings[] = {"0;41", "1;31", "1;33",
                                  "1;32", "1;34", "1;30"};
  printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}
void platform_console_write_error(const char* message, u8 colour) {
  // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
  const char* colour_strings[] = {"0;41", "1;31", "1;33",
                                  "1;32", "1;34", "1;30"};
  printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

void platform_log(LogLevel level,
                  const char* tag,
                  const char* format,
                  va_list args) {
  esp_log_writev(MAP_LOG_LEVELS[static_cast<std::size_t>(level)], tag, format,
                 args);
}

void platform_sleep(u64 ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

// allocation functions for freerots are also mapped to heap_caps_malloc
// so no need to us the freertos functions
// ref:
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html#freertos-heap

// https :  // youtu.be/y4xfub_s7Zk?t=2665
//  esp_log_write() esp_log_writeev()

}  // namespace hakkou
#endif