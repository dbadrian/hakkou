#pragma once

#include "defines.h"
#include "logger.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

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

// FreeRTOS wrapped
template <typename ItemType, std::size_t EventQueueSize>
struct Queue {
  u8 buffer[EventQueueSize * sizeof(ItemType)];
  StaticQueue_t queue;
  QueueHandle_t handle;

  Queue() {
    HWARN("QUEUE CREATED");
    handle =
        xQueueCreateStatic(EventQueueSize, sizeof(ItemType), buffer, &queue);
    // TODO: handle better
    configASSERT(handle);
  }

  ~Queue() { HWARN("QUEUE Destroyed"); }

  void send_front(const ItemType& item, TickType_t wait = portMAX_DELAY) {
    //TODO handle failure
    xQueueSendToFront(handle, &item, wait);
  }
  void send_back(const ItemType& item, TickType_t wait = portMAX_DELAY) {
    // TODO handle failure

    xQueueSendToBack(handle, &item, wait);
  }
};

}  // namespace hakkou