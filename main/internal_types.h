#pragma once

#include "defines.h"

#include "sdkconfig.h"

#include <array>
#include <variant>
#include <optional>

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
  ESC,
  OK,
  UP,
  DOWN,
  LEFT,
  RIGHT,
};

enum class RotaryEncoderEvent : u8 {
  LEFT,
  RIGHT,
  BUTTON_PRESSED,
  BUTTON_RELEASED,
  BUTTON_CLICKED,
  BUTTON_LONG_PRESS
};

using TimeString = std::array<char, 8>;


enum class SystemErrors {
  OUT_OF_MEMORY,
  UNKNOWN_FATAL,
};

struct SystemEventError {
  std::optional<SystemErrors> code{std::nullopt};
};
struct SystemEventStart {};
struct SystemEventSensorFailure {};
struct SystemEventCriticalFailure {};
struct SystemEventStartManual {};
struct SystemEventStartProgram {};
struct SystemEventOpenSettings {};
struct SystemEventAbort {};
struct SystemEventNOP {};

using SystemEvent = std::variant<SystemEventError,
                                SystemEventStart,
                                SystemEventSensorFailure,
                                SystemEventCriticalFailure,
                                SystemEventStartManual,
                                SystemEventStartProgram,
                                SystemEventOpenSettings,
                                SystemEventAbort,
                                SystemEventNOP>;

}  // namespace hakkou