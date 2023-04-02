#include "controller.h"

namespace hakkou {

void Controller::clean_up() {
  event_post({
      .event_type = EventType::FanDuty,
      .fan_duty = 0,
  });

  if (hmd_handle) {
    event_unregister(hmd_handle.value());
  }
  if (amb_handle) {
    event_unregister(amb_handle.value());
  }
  if (food_handle) {
    event_unregister(food_handle.value());
  }
  if (system_handle) {
    event_unregister(system_handle.value());
  }
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

}  // namespace hakkou