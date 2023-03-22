#include "event.h"
#include "logger.h"

#include <optional>
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

// Task/Stack buffer
constexpr static uint32_t STACK_SIZE = 2 * 2048;
constexpr static UBaseType_t PRIORITY = 9;  // TODO: set somewhere else
StackType_t task_stack[STACK_SIZE];
StaticTask_t task_buffer;
TaskHandle_t xHandle{nullptr};

//
u32 callback_id_counter = 0;
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
#ifdef CONFIG_FREERTOS_EXTRA_DEBUG
  vQueueAddToRegistry(state.message_queue, "EventMessageQueue");
#endif

  // preallocate some memory of the std::vectors to prevent fragmentation
  // TODO: can we do this with our own pool allocator?
  for (auto& cb_vec : state.callbacks) {
    cb_vec.reserve(EventSystemState::QUEUE_PREALLOCATION_RATE);
  }

  xHandle = xTaskCreateStatic(
      event_loop,    /* Function that implements the task. */
      "events",      /* Text name for the task. */
      STACK_SIZE,    /* Number of indexes in the xStack array. */
      nullptr,       /* Parameter passed into the task. */
      PRIORITY,      /* Priority at which the task is created. */
      task_stack,    /* Array to use as the task's stack. */
      &task_buffer); /* Variable to hold the task's data structure. */

  HINFO("Initialized Event-subsystem")

  is_initialized = true;

  return true;
}

std::optional<EventHandle> event_register(EventType event_type,
                                          void* listener,
                                          OnEventCallback callback) {
  if (!is_initialized) {
    return std::nullopt;
  }

  // check if something with the same "signature" was already added
  auto signature = std::tie(callback, listener);
  auto cb_vec = state.callbacks[static_cast<std::size_t>(event_type)];
  for (const auto& cbr : cb_vec) {
    if (std ::tie(cbr.callback, cbr.listener) == signature) {
      return std::nullopt;
    }
  }

  u32 id = ++callback_id_counter;
  cb_vec.push_back(EventCallbackRegistry{
      .listener = listener, .callback = callback, .id = id});

  return {.event_type = event_type, .id = id};
}

bool event_unregister(EventHandle handle) {
  if (!is_initialized) {
    return false;
  }

  auto erased = std::erase_if(
      state.callbacks[static_cast<std::size_t>(handle.event_type)],
      [&signature](EventCallbackRegistry& x) { return x.id == handle.id; });

  return erased > 0
}

bool event_post(Event event,
                TickType_t wait = portMAX_DELAY,
                bool high_priority = false) {
  if (!is_initialized) {
    return false;
  }

  bool ret = pdTRUE;
  if (high_priority) {
    // If a high priority event is posted directly after another
    // it would queue before the older message, which is generally not desirable
    // Hence, hence use with caution until a different system is developed. :)
    ret = xQueueSendToFront(state.message_queue, &event, wait);
  } else {
    ret = xQueueSendToBack(state.message_queue, &event, wait);
  }

  if (ret == pdTRUE) {
    HDEBUG("Posted event to queue.");
    return true;
  } else {
    HWARN("Failed to post event to queue.")
    return false;  // failed to post message within the defined wait period
  }
}

void event_loop(void* params) {
  // Note(David): How could we have message priorities and wait for all
  Event event;

  HDEBUG("Event loop started.")
  while (true) {
    // Since wait time is set to `portMAX_DELAY`, the cmd waits indefinitely for
    // a message hence should ways return true
    // TODO: Check if we can get rid of this, or if for other reasons it might
    // fail to return correctly, however, we will always exit then
    if (xQueueReceive(state.message_queue, static_cast<void*>(&event),
                      portMAX_DELAY) != pdTRUE) {
      break;
    }

    // Cache the relevant vector of callbacks
    const auto& cb_vec =
        state.callbacks[static_cast<std::size_t>(event.event_type)];

    // No callback was defined for this event_type so we can skip this event
    if (cb_vec.empty()) {
      // TODO(DBA): Should this be a warning or rather debug? When would we want
      // to emit events without a callback defined? Maybe the IR cmds or
      // others???
      HWARN("Received event (type: %i) without callbacks.")
      continue;
    }

    // Pass event to each callback. In particular, callbacks can return a stop
    // signal to notify the event subsystem, to not forward the event to any
    // further callbacks.
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
  HDEBUG("Event loop finished.")

  // TODO: here we should delete the task, but really it should never end
  // so we could use the exception raised to cause a system restart
  // just a bit of hidden behaviour?
  vTaskDelete(NULL);
}

class ScopedEventHandleGuard {
 public:
  HandleGuard(EventHandle handle) : handle_(handle) {}
  ~HandleGuard() { event_unregister(handle_); }

 private:
  EventHandle handle_;
};

}  // namespace hakkou