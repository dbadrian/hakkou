#pragma once

#include <event.h>
#include <logger.h>

#include <encoder.h>

#define RE_A_GPIO gpio_num_t(19)
#define RE_B_GPIO gpio_num_t(23)
#define RE_BTN_GPIO gpio_num_t(18)
#define EV_QUEUE_LEN 5

namespace hakkou {
void rotary_encoder_task(void* arg) {
  QueueHandle_t event_queue;
  rotary_encoder_t re;

  // Create event queue for rotary encoders
  event_queue = xQueueCreate(EV_QUEUE_LEN, sizeof(rotary_encoder_event_t));

  // Setup rotary encoder library
  ESP_ERROR_CHECK(rotary_encoder_init(event_queue));

  // Add one encoder
  memset(&re, 0, sizeof(rotary_encoder_t));
  re.pin_a = RE_A_GPIO;
  re.pin_b = RE_B_GPIO;
  re.pin_btn = RE_BTN_GPIO;
  ESP_ERROR_CHECK(rotary_encoder_add(&re));

  rotary_encoder_event_t e;
  int32_t val = 0;

  HINFO("Initial value: %" PRIi32, val);
  while (1) {
    xQueueReceive(event_queue, &e, portMAX_DELAY);

    switch (e.type) {
      case RE_ET_BTN_PRESSED:
        HINFO("Button pressed");
        event_post({.event_type = EventType::RotaryEncoder,
                    .sender = nullptr,
                    .rotary_event = RotaryEncoderEvent::BUTTON_PRESSED});
        break;
      case RE_ET_BTN_RELEASED:
        HINFO("Button released");
        event_post({.event_type = EventType::RotaryEncoder,
                    .sender = nullptr,
                    .rotary_event = RotaryEncoderEvent::BUTTON_RELEASED});
        break;
      case RE_ET_BTN_CLICKED:
        HINFO("Button clicked");
        // rotary_encoder_enable_acceleration(&re, 100);
        event_post({.event_type = EventType::RotaryEncoder,
                    .sender = nullptr,
                    .rotary_event = RotaryEncoderEvent::BUTTON_CLICKED});
        break;
      case RE_ET_BTN_LONG_PRESSED:
        HINFO("Looooong pressed button");
        // rotary_encoder_disable_acceleration(&re);
        event_post({.event_type = EventType::RotaryEncoder,
                    .sender = nullptr,
                    .rotary_event = RotaryEncoderEvent::BUTTON_LONG_PRESS});
        break;
      case RE_ET_CHANGED:
        if (e.diff > 0) {
          event_post({.event_type = EventType::RotaryEncoder,
                      .sender = nullptr,
                      .rotary_event = RotaryEncoderEvent::LEFT});

        } else {
          event_post({.event_type = EventType::RotaryEncoder,
                      .sender = nullptr,
                      .rotary_event = RotaryEncoderEvent::RIGHT});
        }
        // val += e.diff;
        // HINFO("Value = %" PRIi32, val);
        break;
      default:
        break;
    }
  }
}
}  // namespace hakkou