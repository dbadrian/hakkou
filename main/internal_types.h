#pragma once

#include "defines.h"

#include "sdkconfig.h"

#include <array>

namespace hakkou {

// Using raw c-arrays in a bit of a pain to work with due to lack of value
// semantic. not necessary, but its meant to be for convenience.
using ScreenData =
    std::array<std::array<char, CONFIG_LCD_COLS + 1>, CONFIG_LCD_ROWS>;

struct NECScanCode {
  u16 address;
  u16 command;
  bool is_repeated;
};

enum GUIEvent : u16 {
  GUI_ONOFF,
  GUI_ESC,
  GUI_OK,
  GUI_UP,
  GUI_DOWN,
  GUI_LEFT,
  GUI_RIGHT,
};

using TimeString = std::array<char, 8>;

}  // namespace hakkou