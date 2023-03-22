#pragma once
#include <numeric>

namespace hakkou {

template <typename T, int window_size>
class SlidingWindowAccumulator {
  // Also serves as circular buffer
 public:
  void push(T val) {
    readings_[idx] = val;
    idx = (idx + 1) % window_size;
  }

  // // TODO: use accumulate on the outside
  // // and instead use the the actual indices to expose
  // // .begin and .end iterators -> no more initalization problems as well
  // [[nodiscard]] T accumulate() const {
  //   return std::accumulate(readings_.begin(), readings_.end(), 0.0f) /
  //          readings_.size();
  // }

  // [[nodiscard]] T update_and_accumulate(T val) {
  //   update(val);
  //   return accumulate();
  // }

  [[nodiscard]] T front() const { return readings_[idx]; }

  [[nodiscard]] T back() const {
    return readings_[((idx - 1) + window_size) % window_size];
  }

  [[nodiscard]] std::size_t get_idx() { return idx; }

  void fill(T val) { readings_.fill(val); }

 private:
  std::size_t idx{0};
  std::array<T, window_size> readings_{T{}};
};

}  // namespace hakkou