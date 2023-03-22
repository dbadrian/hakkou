#include "platform.h"

// #ifdef ESP_PLATFORM // TODO: This is not really checking much except that it
// was built with esp-idf...maybe enough?
#if true
#include "logger.h"

#include "esp_log.h"

namespace hakkou {

namespace {
u8 MAP_LOG_LEVELS[static_cast<std::size_t>(LogLevel::MAX_LOG_LEVELS)] = {
    ESP_LOG_ERROR,    // <- LOG_LEVEL_FATAL
    ESP_LOG_ERROR,    // <- LOG_LEVEL_ERROR
    ESP_LOG_WARN,     // <- LOG_LEVEL_WARN
    ESP_LOG_INFO,     // <- LOG_LEVEL_INFO
    ESP_LOG_DEBUG,    // <- LOG_LEVEL_DEBUG
    ESP_LOG_VERBOSE,  // <- LOG_LEVEL_TRACE
};
}

// https :  // youtu.be/y4xfub_s7Zk?t=2665
//  esp_log_write() esp_log_writeev()

}  // namespace hakkou
#endif