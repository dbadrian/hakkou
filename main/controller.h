#pragma once

#include "defines.h"
#include "event.h"
#include "hardware/pid.h"
#include "platform/platform.h"

#include <mutex>

namespace hakkou {

struct SensorStates {
  HMutex mtx;
  float amb_humidity = 0;
  float amb_temperature = 0;
  float food_temperature = 0;
};

// struct ControllerGUIState {
//   // main screen related states
//   bool selected_temp{true};
//   // relating toa bort sceen
//   bool on_abort_screen{false};
//   bool selected_cont{true};
//   HMutex mtx;
// };

class Controller {
 public:
  // PID settings
  // TODO: make configurable
  constexpr static float Kp = 1000.0f;
  constexpr static float Ki = 500.0f;
  constexpr static float Kd = 5.0f;
  constexpr static float tau = 0.0f;
  constexpr static float integrator_min = -1000.0f;
  constexpr static float integrator_max = 1000.0f;
  constexpr static float out_min = 0;
  constexpr static float out_max = 100.0f;

  // Task related
  constexpr static uint32_t STACK_SIZE = 2 * 2048;
  constexpr static UBaseType_t PRIORITY = 15;

  Controller()
      : temperature_pid_(PIDConfig{Kp, Ki, Kd, tau, out_min, out_max,
                                   integrator_min, integrator_max}) {
    // Register for the various relevent events
    //  For dispatching to the respective callback, we use a single
    //  static method `event_callback` that switches to right method
    system_handle = event_register(EventType::System, static_cast<void*>(this),
                                   Controller::event_callback);
    if (!system_handle) {
      HFATAL("Couldn't register controller event callback!");
    }
    hmd_handle =
        event_register(EventType::HumidityAmbient, static_cast<void*>(this),
                       Controller::event_callback);
    if (!hmd_handle) {
      HFATAL("Couldn't register controller event callback!");
    }
    amb_handle =
        event_register(EventType::TemperatureAmbient, static_cast<void*>(this),
                       Controller::event_callback);
    if (!amb_handle) {
      HFATAL("Couldn't register controller event callback!");
    }
    food_handle =
        event_register(EventType::TemperatureFood, static_cast<void*>(this),
                       Controller::event_callback);
    if (!food_handle) {
      HFATAL("Couldn't register controller event callback!");
    }
  }

  void run(TaskHandle_t task_handle) {
    task_handle_ = task_handle;
    run_manual();  // TODO: readd the program mode
    clean_up();
  }

  static CallbackResponse event_callback(Event event, void* listener) {
    auto instance = static_cast<Controller*>(listener);
    switch (event.event_type) {
      case EventType::System: {
        return instance->system_event_cb(event);
      } break;
      case EventType::TemperatureFood: {
        return instance->food_temp_event_cb(event);
      } break;
      case EventType::TemperatureAmbient: {
        return instance->ambient_temp_event_cb(event);
      } break;
      case EventType::HumidityAmbient: {
        return instance->ambient_hmd_cb(event);
      } break;
      default:
        break;
    }
  }

 private:
  CallbackResponse system_event_cb(const Event event) {
    // TODO: ????
    // Mostly responding to system errors
    // -> shutdown heater (although system errors should probably just restart.)
    // e.g., if food sensors are detached?
    return CallbackResponse::Continue;
  }
  CallbackResponse food_temp_event_cb(const Event event) {
    const std::scoped_lock lock(sensor_states_.mtx);
    sensor_states_.amb_temperature = event.temperature;
    return CallbackResponse::Continue;
  }
  CallbackResponse ambient_temp_event_cb(const Event event) {
    const std::scoped_lock lock(sensor_states_.mtx);
    sensor_states_.amb_temperature = event.temperature;
    return CallbackResponse::Continue;
  }
  CallbackResponse ambient_hmd_cb(const Event event) {
    const std::scoped_lock lock(sensor_states_.mtx);
    sensor_states_.amb_humidity = event.humidity;
    return CallbackResponse::Continue;
  }

  void clean_up();

  void run_manual() {
    // const uint32_t start_time_s = get_time_sec();
    uint32_t time_passed_m = 0;
    bool running = true;

    float temp_measured{};
    float hmd_measured{};

    uint32_t temp_pid_duty{};

    while (running) {
    }
  }

  [[nodiscard]] float get_adjusted_temperature(float setpoint) {
    // TODO: get rid of this or reintegrate the adjustment method
    return sensor_states_.food_temperature;
  }

  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;
  TaskHandle_t task_handle_;

  std::optional<EventHandle> hmd_handle;
  std::optional<EventHandle> amb_handle;
  std::optional<EventHandle> food_handle;
  std::optional<EventHandle> system_handle;

  // PID to manage the heater
  PID temperature_pid_;
  float temp_setpoint_{30};
  float hmd_setpoint_{50};

  // single mtx for measurements is enough
  // since the event loop will only process one
  // event at a time
  SensorStates sensor_states_;
};

}  // namespace hakkou