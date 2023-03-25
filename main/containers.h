#pragma once
#include <array>

namespace hakkou {

template <typename T, int window_size>
class SlidingWindowAccumulator {
  // Also serves as circular buffer
 public:
  void push(T val) {
    readings_[idx] = val;
    idx = (idx + 1) % window_size;
  }

  auto begin() { return readings_.begin(); }
  auto end() { return readings_.end(); }

  [[nodiscard]] T front() const { return readings_[idx]; }

  [[nodiscard]] T back() const {
    return readings_[((idx - 1) + window_size) % window_size];
  }

  [[nodiscard]] std::size_t get_idx() { return idx; }

  void fill(T val) { readings_.fill(val); }

  std::size_t size() { return window_size; }

 private:
  std::size_t idx{0};
  std::array<T, window_size> readings_{T{}};
};

}  // namespace hakkou