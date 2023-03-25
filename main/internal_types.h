#pragma once

#include "sdkconfig.h"

#include <array>

namespace hakkou {

// Using raw c-arrays in a bit of a pain to work with due to lack of value
// semantic. not necessary, but its meant to be for convenience.
using ScreenData =
    std::array<std::array<char, CONFIG_LCD_COLS + 1>, CONFIG_LCD_ROWS>;
}  // namespace hakkou