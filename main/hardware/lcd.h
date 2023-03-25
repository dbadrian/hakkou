
#pragma once
#include "defines.h"
#include "logger.h"

// TODO: ESP-IDF specific?
#include <hd44780.h>
#define CONFIG_I2CDEV_NOLOCK
#include <pcf8574.h>

#include <cstring>
#include <string_view>

namespace hakkou {

// // basic interaction functions
// void hd44780_set_character(hd44780_t& lcd,
//                            std::size_t slot,
//                            std::array<uint8_t, 8> character) {
//   hd44780_upload_character(&lcd, slot, character.data());
// }

esp_err_t write_lcd_data_callback(const hd44780_t* lcd, uint8_t data);
hd44780_t create_lcd_hw_connection();

class LCD {
  // TODO MAKE CONFIG
  constexpr static std::size_t Rows = CONFIG_LCD_ROWS;
  constexpr static std::size_t Cols = CONFIG_LCD_COLS;
  constexpr static u32 STACK_SIZE = 2304;
  constexpr static UBaseType_t PRIORITY = 9;

 public:
  LCD() : lcd_(create_lcd_hw_connection()) {
    task_handle_ = xTaskCreateStatic(initialize, "LCD", STACK_SIZE, this,
                                     PRIORITY, task_buf_, &xTaskBuffer);
  }

  void update(const char* text);

  // TODO: This class is not thread-safe
  void clear() { hd44780_clear(&lcd_); }

 private:
  static void initialize(void* cls) { static_cast<LCD*>(cls)->run(); }
  void run() {
    HINFO("LCD task started.");
    u32 notification = 0;

    while (true) {
      notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      if (notification == 1) {
        HDEBUG("Updating LCD");
        // got notified to update LCD
        for (std::size_t i = 0; i < Rows; i++) {
          if (dirty_[i]) {
            hd44780_gotoxy(&lcd_, 0, i);
            hd44780_puts(&lcd_, buf[i]);
          }
        }
      }
    }
  }

  hd44780_t lcd_;
  bool dirty_[Rows]{false};
  char buf[Rows][Cols + 1] = {};

  // task handle used for notifications
  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;
  TaskHandle_t task_handle_;
};

}  // namespace hakkou
