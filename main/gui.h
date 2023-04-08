#pragma once
#include "defines.h"
#include "internal_types.h"
#include "shared.h"

#include <algorithm>
#include <array>
#include <string>

namespace hakkou {

constexpr static auto C_EMPTY = ' ';
constexpr static auto C_SELECTOR_OFF = ' ';
constexpr static auto C_SELECTOR_ON = '>';
constexpr static auto C_SELECTOR_SPECIAL = '-';
constexpr static auto C_UP = '^';
constexpr static auto C_DOWN = 'v';

class Screen {
 public:
  constexpr static u8 Rows = CONFIG_LCD_ROWS;
  constexpr static u8 Cols = CONFIG_LCD_COLS;

  virtual const ScreenData& data() const;
};

class AbortScreen : public Screen {
 public:
  const ScreenData& data() const override { return screen_buf_; }

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

 protected:
  ScreenData screen_buf_ = {{{"                    "},
                             {"      > CONT. <     "},
                             {"        ABORT       "},
                             {"                    "}}};
};

enum class ProgressScreenState { ON_SCREEN, ON_ABORT_SCREEN, ABORTED };

class ProgressScreen : public Screen {
 public:
  const ScreenData& data() const override { return screen_buf_; }

  void update(float amb_temp,
              float amb_hmd,
              float temp_setpoint,
              float hmd_setpoint,
              std::optional<float> food_temp,
              bool food_is_control,
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
    if (total_time) {
      sprintf(screen_buf_[3].data(), " %.7s / %.7s ",
              time_to_string_representation(time_passed).data(),
              time_to_string_representation(total_time.value()).data());
    } else {
      sprintf(screen_buf_[3].data(), "       %.7s",
              time_to_string_representation(time_passed).data());
    }

    if (food_is_control) {
      screen_buf_[0].data()[0] = '*';
      screen_buf_[1].data()[0] = ' ';
    } else {
      screen_buf_[0].data()[0] = ' ';
      screen_buf_[1].data()[0] = '*';
    }

    // if (time_passed && total_time) {
    //   sprintf(progress_screen_buf_[3], " %s / %s ",
    //           time_to_string_representation(time_passed.value()).c_str(),
    //           time_to_string_representation(total_time.value()).c_str());
    // }
  }

  ScreenData screen_buf_ = {{{"                    "},
                             {"                    "},
                             {"                    "},
                             {"                    "}}};
};

template <std::size_t N>
class ScrollableListScreen : public Screen {
 public:
  u16 selected_item{0};

 public:
  ScrollableListScreen(std::array<std::string, N>&& items)
      : items_(items) {
    update();
  };

  const ScreenData& data() const override { return screen_buf_; }
  void up() {
    if (selected_item > 0) {
      // ensure we dont wrap around selected_item by accident
      selected_item = std::max(0, selected_item - 1);
    }
    if (selected_item < cursor_pos_) {
      // if the currently selected item index is smaller than cursor (e.g., 0 vs
      // 1) we need to "scroll" the list up
      cursor_pos_--;
    }
    update();
  }

  void down() {
    selected_item = std::min(static_cast<std::size_t>(selected_item + 1),
                             items_.size() - 1);
    if (selected_item > cursor_pos_ + 3) {
      // if the currently selected item index is smaller than cursor (e.g., 0 vs
      // 1) we need to "scroll" the list up
      cursor_pos_++;
    }
    update();
  }

 private:
  void update() {
    // fill
    for (std::size_t i = 0; i < Rows; i++) {
      std::size_t item_id_with_offset = i + cursor_pos_;

      if (item_id_with_offset >= items_.size()) {
        // if less items are added to the list -> no scrolling etc.
        break;
      }
      sprintf(screen_buf_[i].data(), " %-18s",
              items_[item_id_with_offset].substr(0, 18).c_str());
    }

    // draw selected row indicator
    std::size_t selected_row = selected_item - cursor_pos_;
    screen_buf_[selected_row][0] = C_SELECTOR_ON;

    // draw scroll cursor indicator
    if (cursor_pos_ > 0) {
      screen_buf_[0][Cols - 1] = C_UP;
    } else {
      screen_buf_[0][Cols - 1] = ' ';
    }

    if (items_.size() > Rows && cursor_pos_ + 3 < items_.size() - 1) {
      screen_buf_[Rows - 1][Cols - 1] = C_DOWN;
    } else {
      screen_buf_[Rows - 1][Cols - 1] = ' ';
    }
  }

 private:
  const std::array<std::string, N> items_;
  u16 cursor_pos_{0};
  // Defaut initialize with valid/null terminated strings!
  ScreenData screen_buf_ = {{{"                    "},
                             {"                    "},
                             {"                    "},
                             {"                    "}}};
};

}  // namespace hakkou