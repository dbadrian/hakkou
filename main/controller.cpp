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

}  // namespace hakkou