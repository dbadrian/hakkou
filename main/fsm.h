// Directly based of https://github.com/mpusz/fsm-variant
// with heavy extension and modification. Full original license reproduced in
// full below.
#pragma once

#include <defines.h>
#include <platform/platform.h>

#include <string_view>

#include <memory>
#include <optional>
#include <utility>

namespace hakkou {

// Common events that can be reused
namespace fsm {
template <typename Derived, typename StateVariant>
class FSM {
 protected:
  StateVariant state_;

  HMutex mtx_;
  TaskHandle_t handle_{nullptr};

 public:
  FSM(){};

  [[nodiscard]] const StateVariant& get_state() const { return state_; }
  //   StateVariant &get_state() { return state_; }

  void notify_current_task(u32 value) {
    if (handle_) {
      xTaskNotify(handle_, value, eSetValueWithOverwrite);
    }
  }

  template <typename T>
  void task_cleanup(T& s) {
    if constexpr (requires { T::mtx; }) {
      mtx_.lock();
      HDEBUG("[FSM_BASE] Acquired mtx");
    }
    if (handle_) {
      handle_ = nullptr;
    }
    if constexpr (requires { T::mtx; }) {
      mtx_.unlock();
      HDEBUG("[FSM_BASE] Released mtx");
    }
  }

  template <typename Event>
  void dispatch(Event&& event) {
    Derived& child = static_cast<Derived&>(*this);
    auto new_state = std::visit(
        [&](auto& s) -> std::optional<StateVariant> {
          return child.on_event(s, std::forward<Event>(event));
        },
        state_);
    if (new_state) {
      state_ = *std::move(new_state);
    }
  }
};

template <class T>
using UP = typename std::unique_ptr<T>;

struct event_start {};

// some gui related events
struct event_gui_ok {};
struct event_gui_esc {};
struct event_gui_right {};
struct event_gui_right {};
struct event_gui_up {};
struct event_gui_down {};

template <typename Error_T>
struct event_error {
  std::optional<Error_T> code{std::nullopt};
};

}  // namespace fsm

}  // namespace hakkou

// MIT License

// Copyright (c) 2017 Mateusz Pusz

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
