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
  PID_SetLevel,
  FAN_RPM,
  FAN_SetLevel,
  //
  NUM_EVENTS,
};

// 32 bit
union EventContext {
  // readout from the 4pin fan
  struct FanRPM {
    u16 value;
  } rpm;
  // any temperature sensor
  struct TemperatureReading {
    f32 value;
  } temperature;
  // any humidity reading
  struct HumidityReading {
    f32 value;
  } humidity;
  // system codes (mostly errors)
  struct SystemCode {
    u16 code;
  } code;
};

// tagged union approach
struct Event {
  EventType event_type;
  void* sender = nullptr;
  EventContext context;
};
// TODO: Static assert the sizeevent_initialize(

// Function signature
enum class CallbackRespone : u8 {
  STOP,
  CONTINUE,
  //
  NUM_CALLBACK_RESPONSES,
};

using OnEventCallback = CallbackRespone (*)(Event event, void* listener);

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

// // what size is std:;size_t on the esp32? whats the native type?
// enum class EventPriority : u8 {
//   LOW,   // -> send to back of queue
//   HIGH,  // -> send to front of queue ...however then other high prio tasks
//   will
//          // be pushed back
//   NUM_PRIORITIES,
// };

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

bool event_fire(Event event, void* sender);

void event_loop(void* params);

}  // namespace hakkou