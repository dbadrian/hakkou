#pragma once

#include <containers.h>
#include <defines.h>
#include <event.h>
#include <internal_types.h>
#include <platform/platform.h>
#include "controller.h"
#include "hardware/fan.h"
#include "hardware/h_ds18x20.h"
#include "hardware/heater.h"
#include "hardware/sht31d.h"
#include "ota.h"

// different state/routines
#include <states/main_menu.h>

#include <string_view>

#include <memory>
#include <optional>
#include <utility>

namespace hakkou {

template <typename Derived, typename States, typename Events>
class FSMBase {
  // This strongly inspired by various talks/blogs on how to use
  // std::variant for building an FSM.
  // This is most inspired by https://github.com/mpusz/fsm-variant
  // However, it is neither directly build nor compatible with what you see
  // there Primarily, there are various changes:
  //  - more/direct integration with the event system of this project
  // Events are also done via a variant-type, to integrate it into the
  // existing event system
 protected:
  States state_;

  HMutex task_mtx_;
  TaskHandle_t task_handle_{nullptr};

 public:
  FSMBase() {};

  [[nodiscard]] const States& get_state() const { return state_; }

  void dispatch(const Events& event) {
    Derived& child = static_cast<Derived&>(*this);
    auto new_state = std::visit(
        [&](auto& s, auto& ev) -> std::optional<States> {
          return child.on_event(s, ev);
        },
        state_, event);
    if (new_state) {
      state_ = *std::move(new_state);
    }
  }

  void notify_current_task(u32 value) {
    if (task_handle_) {
      xTaskNotify(task_handle_, value, eSetValueWithOverwrite);
    }
  }

  template <typename T>
  void task_cleanup(T& s) {
    if constexpr (requires { T::mtx; }) {
      task_mtx_.lock();
      HDEBUG("[FSM_BASE] Acquired mtx");

      task_handle_ = nullptr;

      task_mtx_.unlock();
      HDEBUG("[FSM_BASE] Released mtx");
    }
  }
};

struct StateIdle {};
struct StateMainMenu {
  // Handle to task started for this state
  // needs to be shutdown on transition!
  std::unique_ptr<MainMenu> menu;
};
struct StateManualRun {
  std::unique_ptr<Controller> ctrl;
};
struct StateProgrammedRun {};
struct StateSettings {};
struct StateFailure {};
struct StateShutdown {};
struct StateFinished {};
struct StateOTA {};

using StatesT = std::variant<std::unique_ptr<StateIdle>,
                             std::unique_ptr<StateMainMenu>,
                             std::unique_ptr<StateManualRun>,
                             std::unique_ptr<StateProgrammedRun>,
                             std::unique_ptr<StateSettings>,
                             std::unique_ptr<StateFailure>,
                             std::unique_ptr<StateShutdown>,
                             std::unique_ptr<StateOTA>,
                             std::unique_ptr<StateFinished>>;

class MainMenuFSM : public FSMBase<MainMenuFSM, StatesT, SystemEvent> {
 private:
  constexpr static u16 EVENT_QUEUE_SIZE = 10;
  Queue<SystemEvent, EVENT_QUEUE_SIZE> event_queue_;
  EventHandle event_handle_;

 public:
  MainMenuFSM() {
    // initial state.
    state_ = std::make_unique<StateIdle>();

    auto event_handle =
        event_register(EventType::System, static_cast<void*>(this),
                       MainMenuFSM::event_callback);
    if (!event_handle) {
      HFATAL("Couldn't register FSM event callback!");
    } else {
      event_handle_ = event_handle.value();
    }
  }

  ~MainMenuFSM() { event_unregister(event_handle_); }

  static CallbackResponse event_callback(Event event, void* listener) {
    auto inst = static_cast<MainMenuFSM*>(listener);
    inst->event_queue_.send_back(event.system_event);
    return CallbackResponse::Continue;
  }

  void run() {
    SystemEvent event;
    while (true) {
      // check for notifcations to kill itself

      if (xQueueReceive(event_queue_.handle, static_cast<void*>(&event),
                        portMAX_DELAY) != pdTRUE) {
        break;
      }
      HDEBUG("[FSM] Got an event....");
      dispatch(event);
    }
  }

  template <typename State, typename Event>
  auto on_event(State&, const Event&) {
    HERROR("Invalid transition\n");
    return std::nullopt;
  }

  template <typename State>
  auto on_event(State&, const SystemEventCriticalFailure& err) {
    HDEBUG("<Any|SystemEventCriticalFailure|StateFailure>\n");
    HERROR("System Failure Occured\n");
    // switch (err.code) {
    //   default:
    //     break;  // TODO Fill out
    // }
    // TODO: Run halting stuff.
    return std::make_unique<StateFailure>();
  }

  auto on_event(std::unique_ptr<StateIdle>&, const SystemEventStart&) {
    HDEBUG("<StateIdle|SystemEventStart|StateMainMenu>");
    auto ns = std::make_unique<StateMainMenu>();
    ns->menu = std::make_unique<MainMenu>();
    return ns;
  }

  auto on_event(std::unique_ptr<StateMainMenu>&,
                const SystemEventStartManual&) {
    HDEBUG("<StateMainMenu|SystemEventStartManual|StateManualRun>\n");

    Fan4W* fan = new Fan4W(CONFIG_FAN_PWM_PIN, CONFIG_FAN_TACHO_PIN);
    event_post(
        {.event_type = EventType::FanDuty, .sender = nullptr, .fan_duty = 30});
    Heater* heater = new Heater(CONFIG_HEATER_PWM_PIN);

    // BME280* bme = new
    // BME280(static_cast<i2c_port_t>(CONFIG_BMP280_I2C_ADDRESS),
    //                          static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
    //                          static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN),
    //                          2000);

    SHT31D* whatever =
        new SHT31D(static_cast<i2c_port_t>(0x44),
                   static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
                   static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN), 2000);

    DS18X20* ds18x20 =
        new DS18X20(static_cast<gpio_num_t>(CONFIG_ONEWIRE_PIN), 2000);

    // GPIO for humidifier
    gpio_configure({
        .pin = 4,
        .direction = GPIODirection::OUTPUT,
        .pull_mode = GPIOPullMode::UP,
    });

    auto ns = std::make_unique<StateManualRun>();
    ns->ctrl = std::make_unique<Controller>();
    return ns;
  }

  auto on_event(std::unique_ptr<StateMainMenu>&, const SystemEventOTA&) {
    HDEBUG("<StateMainMenu|SystemEventStartManual|StateOTA>\n");
    auto ns = std::make_unique<StateOTA>();
    xTaskCreate(ota_server_task, "ota_server_task", 8192, NULL, 5, NULL);

    return ns;
  }

  auto on_event(std::unique_ptr<StateManualRun>&, const SystemEventAbort&) {
    HDEBUG("<SystemEventStartManual|SystemEventAbort|StateShutdown>\n");

    event_post(Event{
        .event_type = EventType::ScreenUpdate,
        .screen_data = {{{"   <  SHUTDOWN  >   "},
                         {"                    "},
                         {"   Manual Restart   "},
                         {"      Required      "}}},
    });

    return std::make_unique<StateShutdown>();
  }

  // template <typename State>
  // auto on_event(State&, const event_error& err) {
  //   HERROR("Error Occurred\n");
  //   switch (err.code) {
  //     default:
  //       break;  // TODO Fill out
  //   }
  //   return std::nullopt;
  // }

  // auto on_event(UP<StateIdle>&, const event_start&) {
  //   HDEBUG("Transition <StateIdle, event_start>::StateMenu\n");

  //   auto ns = std::make_unique<StateMainMenu>();
  //   ns->run();
  //   return ns;
  // }

  // state on_event(UP<StateMainMenu>& s, const event_ok&) {
  //   HDEBUG("Transition <StateMainMenu, event_ok>->?\n");
  //   // will wait and block for the task to be finished...
  //   task_cleanup(*s);
  // }
};

}  // namespace hakkou
