#pragma once

#include "defines.h"

#include "freertos/FreeRTOS.h" 
#include "freertos/queue.h"

#include <array>
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

typedef struct EventCallbackRegistry {
  void* listener;  // potentially the class that will receive this callback
  OnEventCallback callback;  // cb that will receive the event
} EventCallbackRegistry;

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

/**
 * Register to listen for when events are sent with the provided code. Events
 * with duplicate listener/callback combos will not be registered again and will
 * cause this to return FALSE.
 * @param code The event code to listen for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function pointer to be invoked when the event
 * code is fired.
 * @returns TRUE if the event is successfully registered; otherwise false.
 */
bool event_register(EventType event_type,
                    void* listener,
                    OnEventCallback callback);

/**
 * Unregister from listening for when events are sent with the provided code. If
 * no matching registration is found, this function returns FALSE.
 * @param code The event code to stop listening for.
 * @param listener A pointer to a listener instance. Can be 0/NULL.
 * @param on_event The callback function pointer to be unregistered.
 * @returns TRUE if the event is successfully unregistered; otherwise false.
 */
bool event_unregister(u16 code, void* listener, OnEventCallback callback);

/**
 * Fires an event to listeners of the given code. If an event callback returns
 * TRUE, the event is considered handled and is not passed on to any more
 * listeners.
 * @param event The event data.
 * @param sender A pointer to the sender. Can be 0/NULL.
 * @returns TRUE if handled, otherwise FALSE.
 */
bool event_fire(Event event, void* sender);

}  // namespace hakkou