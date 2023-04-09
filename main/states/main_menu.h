#pragma once

#include "containers.h"
#include "defines.h"
#include "event.h"
#include "gui.h"
#include "internal_types.h"
#include "platform/platform.h"
#include "tasks.h"

namespace hakkou {

class MainMenu {
  enum class Actions : u8 { OK, UP, DOWN };

 public:
  MainMenu() {  //  : task_(initialize, "MainMenu", 10, this)
    handle = xTaskCreateStatic(
        initialize,            /* Function that implements the task. */
        "MainMenu",      /* Text name for the task. */
        3*1024,       /* Number of indexes in the xStack array. */
        this,      /* Parameter passed into the task. */
        10,        /* Priority at which the task is created. */
        buffer,          /* Array to use as the task's stack. */
        &task_internal); /* Variable to hold the task's data structure. */

    auto _handle = event_register(EventType::GUI, static_cast<void*>(this),
                                 MainMenu::event_handler);
    if (!_handle) {
      HFATAL("Couldn't register MainMenu event callback!");
    }
    gui_handle = _handle.value();
  }

  ~MainMenu() {
    event_unregister(gui_handle);
  }

  static CallbackResponse event_handler(Event event, void* listener) {
    auto instance = static_cast<MainMenu*>(listener);
    switch (event.gui_event) {
      case GUIEvent::OK: {
        instance->queue_.send_back(Actions::OK);
      } break;
      case GUIEvent::UP: {
        instance->queue_.send_back(Actions::UP);
      } break;
      case GUIEvent::DOWN: {
        instance->queue_.send_back(Actions::DOWN);
      } break;
      default:
        break;
    }
    return CallbackResponse::Continue;
  }

  static void initialize(void* cls) { static_cast<MainMenu*>(cls)->run(); }

  void run() {
    event_post(Event{.event_type = EventType::ScreenUpdate,
                     .screen_data = screen.data()});

    bool running = true;
    Actions ev;
    while (running) {
      if (xQueueReceive(queue_.handle, static_cast<void*>(&ev),
                        portMAX_DELAY) != pdTRUE) {
        break;
      }
      switch (ev) {
        case Actions::UP: {
          screen.up();
          event_post(Event{.event_type = EventType::ScreenUpdate,
                           .screen_data = screen.data()});
        } break;
        case Actions::DOWN: {
          screen.down();
          event_post(Event{.event_type = EventType::ScreenUpdate,
                           .screen_data = screen.data()});
        } break;
        case Actions::OK: {
          running = false;
          // communicate to FSM what was selected
          switch (screen.selected_item) {
            case 0: {
              event_post(Event{
                  .event_type = EventType::System,
                  .system_event = SystemEventStartManual{},
              });
            } break;
            case 1: {
              // event_post(Event{
              //     .event_type = EventType::System,
              //     .system_event = SystemEventStartManual{},
              // });
            } break;
            case 2: {
              event_post(Event{
                  .event_type = EventType::System,
                  .system_event = SystemEventOpenSettings{},
              });
            } break;
          }
        } break;
        default:
          break;
      }
    }

    vTaskDelete(NULL);
  }

 private:
  // the on screen selection is selected_pos - scroll_pos
  // constexpr static std::array<std::string, 3> options;

  StackType_t buffer[3*1024];
  StaticTask_t task_internal;
  TaskHandle_t handle;

  // internal state
  EventHandle gui_handle;

  Queue<Actions, 10> queue_;

  ScrollableListScreen<3> screen{{
      "Manual Mode",
      "Programmed Mode",
      "Settings",
  }};
  // Task<4 * 1024> task_;
};

}  // namespace hakkou