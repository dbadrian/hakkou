#pragma once

#include "defines.h"
#include "event.h"
#include "gui.h"
#include "hardware/pid.h"
#include "platform/platform.h"

#include <mutex>

namespace hakkou {

struct SensorStates {
  HMutex mtx;
  float amb_humidity{0};
  float amb_temperature{0};
  float food_temperature{0};
};

struct ControllerGUIState {
  // general
  HMutex mtx;
  // progress screen state
  bool selected_temp{true};
  // abort sceen state
  bool on_abort_screen{false};
  bool selected_cont{true};
};

enum class ControllerMessages : u32 {
  PROCESS_DIE = 1,  // has to be >0
};

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
  constexpr static u64 SLEEP_MS = 50;

  // GUI Queue
  constexpr static u8 GUI_QUEUE_LENGTH = 5;
  constexpr static u8 GUI_QUEUE_ITEM_SIZE = sizeof(Event::gui_event);

  // Default values
  constexpr static float DEFAULT_TEMP_SETPOINT = 25;
  constexpr static float DEFAULT_HMD_SETPOINT = 50;

  Controller()
      : temperature_pid_(PIDConfig{Kp, Ki, Kd, tau, out_min, out_max,
                                   integrator_min, integrator_max}) {
    gui_event_queue_ =
        xQueueCreateStatic(GUI_QUEUE_LENGTH, GUI_QUEUE_ITEM_SIZE,
                           gui_queue_storage, &gui_queue_internal_);

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

    gui_handle = event_register(EventType::GUI, static_cast<void*>(this),
                                Controller::event_callback);
    if (!gui_handle) {
      HFATAL("Couldn't register controller event callback!");
    }

    HINFO("Initialized controller.");
  }

  void run() {
    task_handle_ = xTaskGetCurrentTaskHandle();
    run_manual();  // TODO: read the program mode
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
      case EventType::GUI: {
        return instance->gui_event_cb(event);
      } break;
      default:
        return CallbackResponse::Continue;
    }
  }

 private:
  // Will unregister handles etc.
  void clean_up();

  CallbackResponse system_event_cb(const Event event) {
    // TODO: ????
    // Mostly responding to system errors
    // -> shutdown heater (although system errors should probably just restart.)
    // e.g., if food sensors are detached?
    return CallbackResponse::Continue;
  }

  CallbackResponse food_temp_event_cb(const Event event);
  CallbackResponse ambient_temp_event_cb(const Event event);
  CallbackResponse ambient_hmd_cb(const Event event);
  CallbackResponse gui_event_cb(const Event event);

  void run_manual() {
    const u32 start_time_s = get_time_sec();
    u32 time_passed_m = 0;
    bool running = true;

    float temp_measured{};
    // float hmd_measured{};

    u32 temp_pid_duty{};

    GUIEvent gui_event;
    active_screen_ = &progress_screen_;
    while (running) {
      // TODO 1. check if errors occurred?

      // Process any available GUI event (no wait)
      while (xQueueReceive(gui_event_queue_, static_cast<void*>(&gui_event),
                           0) == pdTRUE) {
        // only update the content of the screens
        // actual update event will be sent later in this loop
        if (gui_state_.on_abort_screen) {
          handle_abort_screen_events(gui_event);
        } else {
          handle_main_screen_events(gui_event);
        }
      }

      // 2. get current time and update passed time
      time_passed_m = (get_time_sec() - start_time_s) / 60;
      HTRACE("time_passed_m=%lu", time_passed_m);

      // 4. Update temperature PID and emit control signals
      // Update the temperature setpoint but querying the
      // latest (adjusted) temperature, putting it to the PID
      // and then publishing it
      temp_measured = get_adjusted_temperature(temp_setpoint_);
      temp_pid_duty = temperature_pid_.update(temp_setpoint_, temp_measured);
      HTRACE("TEMP_PID: setpoint='%f' measured='%f' duty='%lu'", temp_setpoint_,
             temp_measured, temp_pid_duty);
      event_post({.event_type = EventType::FanDuty, .fan_duty = 100});
      event_post(
          {.event_type = EventType::HeaterDuty, .heater_duty = temp_pid_duty});

      // TODO: 5. Humidity: Not implemented!

      progress_screen_.update(
          sensor_states_.amb_temperature,  /* float amb_temp*/
          sensor_states_.amb_humidity,     /* float amb_hmd*/
          temp_setpoint_,                  /* float temp_setpoint*/
          hmd_setpoint_,                   /* float hmd_setpoint*/
          sensor_states_.food_temperature, /* float food_temp*/
          gui_state_.selected_temp,
          time_passed_m * 60, /* uint32_t time_passed*/
          60 * 60
          // program::total_run_time(prgm) * 60 /* uint32_t total_time*/
          // std::nullopt
      );

      // Update and show whatever screen is currently showing
      {
        const std::scoped_lock lock(gui_state_.mtx);
        event_post(Event{.event_type = EventType::ScreenUpdate,
                         .screen_data = active_screen_->data()});
      }

      // Check if we received notification to die
      auto notification = ulTaskNotifyTake(pdTRUE, 0);
      if (notification == static_cast<u32>(ControllerMessages::PROCESS_DIE)) {
        HDEBUG("[Controller] Received kill notifcation %lu", notification);
        break;
      }

      // TODO: measure passed time and sleep accordingly for fixed freq
      platform_sleep(SLEEP_MS);
    }
  }

  [[nodiscard]] float get_adjusted_temperature(float setpoint) {
    // TODO: get rid of this or reintegrate the adjustment method
    return sensor_states_.food_temperature;
  }

  // void gui_event_cb(const Event event) {
  //   auto event = std::get<EVENTS_T>(msg);

  //   if (gui_state_.on_abort_screen) {
  //     handle_abort_screen_events(event);
  //   } else {
  //     handle_main_screen_events(event);
  //   }
  // }

  void handle_abort_screen_events(GUIEvent event);
  void handle_main_screen_events(GUIEvent event);

  // Task/Stack buffer
  StackType_t task_buf_[STACK_SIZE];
  StaticTask_t xTaskBuffer;
  TaskHandle_t task_handle_;

  std::optional<EventHandle> hmd_handle;
  std::optional<EventHandle> amb_handle;
  std::optional<EventHandle> food_handle;
  std::optional<EventHandle> system_handle;
  std::optional<EventHandle> gui_handle;

  // PID to manage the heater
  PID temperature_pid_;
  float temp_setpoint_{DEFAULT_TEMP_SETPOINT};
  float hmd_setpoint_{DEFAULT_HMD_SETPOINT};

  // single mtx for measurements is enough
  // since the event loop will only process one
  // event at a time
  SensorStates sensor_states_;

  // GUI
  Screen* active_screen_ =
      nullptr;  // will be used to switch whats being sent to LCD
  ControllerGUIState gui_state_;
  ProgressScreen progress_screen_;
  AbortScreen abort_screen_;
  u8 gui_queue_storage[GUI_QUEUE_LENGTH * GUI_QUEUE_ITEM_SIZE];
  StaticQueue_t gui_queue_internal_;
  QueueHandle_t gui_event_queue_;
};

}  // namespace hakkou