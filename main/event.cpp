#include "event.h"

#include <tuple>
#include <vector>

namespace hakkou {

// Note: The anonymous namespace keeps this implementation local to this
// translation unit
namespace {
/**
 * Event system internal state.
 */
static bool is_initialized = false;
static EventSystemState state;
}  // namespace

bool event_initialize() {
  if (is_initialized == true) {
    return false;
  }
  is_initialized = false;

  // static initialize of the queue
  state.message_queue = xQueueCreateStatic(
      EventSystemState::QUEUE_LENGTH, EventSystemState::ITEM_SIZE,
      state.queue_storage, &state.queue_internal);
  configASSERT(state.message_queue);  // todo :replace with a custom freertos
                                      // assert call that wraps our asssert

// If RTOS aware kernel debugger is used, this will add meaningful name/
// https://www.freertos.org/vQueueAddToRegistry.html
#ifdef FREERTOS_EXTRA_DEBUG
  vQueueAddToRegistry(state.message_queue, "EventMessageQueue");
#endif

  // preallocate some memory of the std::vectors to prevent fragmentation
  // TODO: can we do this with our own pool allocator?
  for (auto& cb_vec : state.callbacks) {
    cb_vec.reserve(EventSystemState::QUEUE_PREALLOCATION_RATE);
  }

  // TODO: start the event loop here

  is_initialized = true;

  return true;
}

bool event_register(EventType event_type,
                    void* listener,
                    OnEventCallback callback) {
  if (!is_initialized) {
    return false;
  }

  state.callbacks[static_cast<std::size_t>(event_type)].push_back(
      EventCallbackRegistry{.listener = listener, .callback = callback});

  return true;
}

bool event_unregister(EventType event_type,
                      void* listener,
                      OnEventCallback callback) {
  if (!is_initialized) {
    return false;
  }

  auto erased = std::erase_if(
      state.callbacks[static_cast<std::size_t>(event_type)],
      [&callback, &listener](EventCallbackRegistry& x) {
        return std::tie(x.callback, x.listener) == std::tie(callback, listener);
      });

  if (erased > 0) {
    return true;
  } else {
    return false;
  }
}

bool event_post(Event event, TickType_t wait = portMAX_DELAY) {
  if (!is_initialized) {
    return false;
  }

  if (xQueueSendToBack(state.message_queue, &event, wait) != pdPASS) {
    return false;  // failed to post message within the defined wait period
  }

  // posted message to queue
  return true;
}


void event_loop(void*) {
  // Note(David): How could we have message priorities and wait for all
  Event event;

  // wait until a message arrives....so basically an eternal loop
  while (xQueueReceive(state.message_queue, static_cast<void*>(&event),
                       portMAX_DELAY) == pdTRUE) {
    // the relevant cb_vec, cache locally
    const auto& cb_vec =
        state.callbacks[static_cast<std::size_t>(event.event_type)];

    // No callback was defined for this event_type.
    if (cb_vec.empty()) {
      continue;
    }

    // go over each registered cb and call -> validate return value for early
    // stopping
    for (auto& registry : cb_vec) {
      using CR = CallbackRespone;
      const CR res = registry.callback(event, registry.listener);
      if (res == CR::STOP) {
        break;
      } else if (res == CR::CONTINUE) {
        continue;
      }
    }

  }

  // TODO: here we should delete the task, but really it should never end
  // so we could use the exception raised to cause a system restart
  // just a bit of hidden behaviour?
}

}  // namespace hakkou