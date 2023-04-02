#pragma once
#include "defines.h"
#include "internal_types.h"
#include "shared.h"

#include <array>

// GUI...well... :D

namespace hakkou {

class Screen {
 public:
  virtual const ScreenData& data() const = 0;
};

class AbortScreen : public Screen {
 public:
  const ScreenData& data() const { return screen_buf_; }

  void select(bool select_continue) {
    if (select_continue) {
      screen_buf_[1][6] = '>';
      screen_buf_[1][14] = '<';
      screen_buf_[2][6] = ' ';
      screen_buf_[2][14] = ' ';
    } else {
      screen_buf_[1][6] = ' ';
      screen_buf_[1][14] = ' ';
      screen_buf_[2][6] = '>';
      screen_buf_[2][14] = '<';
    }
  }

 private:
  ScreenData screen_buf_ = {{{"                    "},
                             {"      > CONT. <     "},
                             {"        ABORT       "},
                             {"                    "}}};
};

enum class ProgressScreenState { ON_SCREEN, ON_ABORT_SCREEN, ABORTED };

class ProgressScreen : public Screen {
 public:
  const ScreenData& data() const { return screen_buf_; }

  void update(float amb_temp,
              float amb_hmd,
              float temp_setpoint,
              float hmd_setpoint,
              std::optional<float> food_temp,
              bool selected_temp,
              uint32_t time_passed,
              const std::optional<uint32_t>& total_time) {
    if (food_temp) {
      sprintf(screen_buf_[0].data(), "  fT: %4.1fC [%4.1f]%c ",
              food_temp.value(), temp_setpoint, selected_temp ? '<' : ' ');
      sprintf(screen_buf_[1].data(), "  aT: %4.1fC         ", amb_temp);
    } else {
      screen_buf_[0] = {};
      sprintf(screen_buf_[1].data(), "  aT: %4.1fC [%4.1f]%c ", amb_temp,
              temp_setpoint, selected_temp ? '<' : ' ');
    }
    sprintf(screen_buf_[2].data(), "  aH: %4.1f%% [%4.1f]%c ", amb_hmd,
            hmd_setpoint, !selected_temp ? '<' : ' ');
    sprintf(screen_buf_[3].data(), " %.7s / %.7s ",
            time_to_string_representation(time_passed).data(),
            time_to_string_representation(total_time.value()).data());

    // if (time_passed && total_time) {
    //   sprintf(progress_screen_buf_[3], " %s / %s ",
    //           time_to_string_representation(time_passed.value()).c_str(),
    //           time_to_string_representation(total_time.value()).c_str());
    // }
  }

 private:
  ScreenData screen_buf_ = {};
};

}  // namespace hakkou