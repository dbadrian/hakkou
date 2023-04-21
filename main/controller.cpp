#include "controller.h"
#include "event.h"
#include "internal_types.h"
#include "logger.h"

namespace hakkou {

void Controller::clean_up() {
  // Reduce fan Output to be a bit more quiet...
  event_post({
      .event_type = EventType::FanDuty,
      .fan_duty = 40,
  });
}

CallbackResponse Controller::food_temp_event_cb(const Event event) {
  const std::scoped_lock lock(sensor_states_.mtx);
  sensor_states_.food_temperature = event.temperature;
  return CallbackResponse::Continue;
}

CallbackResponse Controller::ambient_temp_event_cb(const Event event) {
  const std::scoped_lock lock(sensor_states_.mtx);
  sensor_states_.amb_temperature = event.temperature;
  return CallbackResponse::Continue;
}

CallbackResponse Controller::ambient_hmd_cb(const Event event) {
  const std::scoped_lock lock(sensor_states_.mtx);
  sensor_states_.amb_humidity = event.humidity;
  return CallbackResponse::Continue;
}

CallbackResponse Controller::gui_event_cb(const Event event) {
  gui_event_queue_.send_back(event.gui_event, pdMS_TO_TICKS(10));
  // TODO: handle return errors
  //     if (xQueueSendToBack(gui_event_queue_, , ) != pdTRUE) {
  //   HERROR("Couldn't queue gui event into controller buffer");
  // }
  return CallbackResponse::Continue;
}

void Controller::handle_abort_screen_events(GUIEvent event) {
  auto leave_screen = [this]() {
    // switch screens and reset the respective state
    gui_state_.on_abort_screen = false;
    gui_state_.selected_cont = true;
    {
      const std::scoped_lock lock(gui_state_.mtx);
      active_screen_ = &progress_screen_;
    }
    abort_screen_.select(gui_state_.selected_cont);
  };

  switch (event) {
    case GUIEvent::OK:
      if (gui_state_.selected_cont) {
        leave_screen();
      } else {
        // Notify controller loop itself
        xTaskNotify(task_handle_,
                    static_cast<u32>(ControllerMessages::PROCESS_DIE),
                    eSetValueWithOverwrite);
      }
      break;
    case GUIEvent::UP:
      gui_state_.selected_cont = !gui_state_.selected_cont;
      abort_screen_.select(gui_state_.selected_cont);
      break;
    case GUIEvent::DOWN:
      gui_state_.selected_cont = !gui_state_.selected_cont;
      abort_screen_.select(gui_state_.selected_cont);
      break;
    case GUIEvent::ESC:
      leave_screen();
      break;
    default:
      HDEBUG("Got unsupported command on <abort-screen>: %lx", event);
      break;
  }
}

void Controller::handle_main_screen_events(GUIEvent event) {
  switch (event) {
    case GUIEvent::OK:
      break;
    case GUIEvent::UP: {
      gui_state_.selected_temp = !gui_state_.selected_temp;
    } break;
    case GUIEvent::DOWN: {
      gui_state_.selected_temp = !gui_state_.selected_temp;
    } break;
    case GUIEvent::LEFT: {
      {
        const std::scoped_lock lock(state_mtx_);

        if (gui_state_.selected_temp) {
          temp_setpoint_ = temp_setpoint_ - 0.5 > 0 ? temp_setpoint_ - 0.5 : 0;
        } else {
          hmd_setpoint_ = hmd_setpoint_ - 0.5 > 0 ? hmd_setpoint_ - 0.5 : 0;
        }
      }
    } break;
    case GUIEvent::RIGHT: {
      {
        const std::scoped_lock lock(state_mtx_);
        if (gui_state_.selected_temp) {
          temp_setpoint_ += 0.5;
        } else {
          hmd_setpoint_ += 0.5;
        }
      }
    } break;
    case GUIEvent::ESC: {
      // switch screens
      gui_state_.on_abort_screen = true;
      gui_state_.selected_temp = true;
      {
        const std::scoped_lock lock(gui_state_.mtx);
        active_screen_ = &abort_screen_;
      }
    } break;
    default:
      HDEBUG("Got unsupported command on <progress-screen>: %lx", event);
      break;
  }
}

}  // namespace hakkou