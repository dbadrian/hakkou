#pragma once

#include "defines.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <array>
#include <optional>
#include <vector>

namespace hakkou {

enum class EventType : u16 {
  System,
  HumidityAmbient,
  TemperatureAmbient,
  TemperatureFood,
  PIDSetLevel,
  FanRPM,
  FanDuty,
  //
  NUM_EVENTS,
};

// tagged union approach
struct Event {
  EventType event_type;
  void* sender = nullptr;
  // 32 bit
  union {
    u32 fan_duty;
    u16 fan_rpm;
    f32 temperature;
    f32 humidity;
    u16 code;
  };
};
// TODO: Static assert the sizeevent_initialize(

// Function signature
enum class CallbackResponse : u8 {
  Stop,
  Continue,
  //
  NUM_CALLBACK_RESPONSES,
};

using OnEventCallback = CallbackResponse (*)(Event event, void* listener);

struct EventCallbackRegistry {
  void* listener;  // potentially the class that will receive this callback
  OnEventCallback callback;  // cb that will receive the event
  u32 id;
};

struct EventHandle {
  EventType event_type;
  u32 id;
};

// fwd dec of the handle guard specified in C file as the
// anonynous namespace variable is accessed
class HandleGuard;

struct EventSystemState {
  // Priority list of events. we can send some events with a higher or lower
  // priority this way
  // std::array<QueueHandle_t,
  //            static_cast<std::size_t>(EventPriority::NUM_PRIORITIES)>
  //     message_queue;
  constexpr static UBaseType_t QUEUE_LENGTH = 50;
  constexpr static UBaseType_t ITEM_SIZE = sizeof(Event);
  u8 queue_storage[QUEUE_LENGTH * ITEM_SIZE];
  StaticQueue_t queue_internal;
  QueueHandle_t message_queue;

  // chose something with a fast insert/delete -> maybe list!?
  constexpr static std::size_t QUEUE_PREALLOCATION_RATE = 16;
  std::array<std::vector<EventCallbackRegistry>,
             static_cast<u32>(EventType::NUM_EVENTS)>
      callbacks;
};

bool event_initialize();

[[nodiscard]] std::optional<EventHandle>
event_register(EventType event_type, void* listener, OnEventCallback callback);

bool event_unregister(EventHandle handle);

bool event_post(Event event,
                bool high_priority = false,
                TickType_t wait = portMAX_DELAY);

void event_loop(void* params);

}  // namespace hakkou