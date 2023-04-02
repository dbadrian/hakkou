#include "lcd.h"

namespace hakkou {

namespace {
// Global that will hold connection for the port extender
// Clearly this is suboptimal, but its very unlikely I'll add a second display
// any time soon.
i2c_dev_t lcd_io;
}  // namespace

esp_err_t write_lcd_data_callback(const hd44780_t* lcd, u8 data) {
  return pcf8574_port_write(&lcd_io, data);
}

hd44780_t create_lcd_hw_connection() {
  hd44780_t _lcd{
      .write_cb = write_lcd_data_callback,  // use callback to send data to
      // by I2C GPIO expander
      .pins = {.rs = 0, .e = 2, .d4 = 4, .d5 = 5, .d6 = 6, .d7 = 7, .bl = 3},
      .font = HD44780_FONT_5X8,
      .lines = CONFIG_LCD_ROWS,
      .backlight = true,
  };

  // port extender setup
  // TODO: Configure on the i2c port etc.
  // TODO: Configure the id, i2c pins etc.
  memset(&lcd_io, 0, sizeof(i2c_dev_t));
  // TODO: get the esp-idf casts out
  ESP_ERROR_CHECK(pcf8574_init_desc(
      &lcd_io, 0x27, i2c_port_t(0), static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
      static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN)));
  // manually increase the SCL speed to 1mhz
  // lcd_io.cfg.master.clk_speed = 1'000'000;  // 1mhz
  ESP_ERROR_CHECK(hd44780_init(&_lcd));

  return _lcd;
}

void LCD::update(const ScreenData& data) {
  // TODO: Again...not threadsafe

  // iterate over each line in buf and check if something changed
  // by byte-wise comparison
  std::scoped_lock lock(mtx_);
  for (std::size_t i = 0; i < Rows; i++) {
    dirty_[i] = memcmp(buf[i], data[i].data(), Cols + 1) != 0;
    if (dirty_[i]) {
      memcpy(buf[i], data[i].data(), Cols);
    }
  }

  xTaskNotify(task_handle_, 1, eSetValueWithOverwrite);
}

}  // namespace hakkou