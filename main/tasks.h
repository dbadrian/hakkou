#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace hakkou {

template <std::size_t StackSize>
struct Task {
  Task(TaskFunction_t task,
       const char* task_name,
       UBaseType_t priority,
       void* parameters = nullptr) {
    handle = xTaskCreateStatic(
        task,            /* Function that implements the task. */
        task_name,       /* Text name for the task. */
        StackSize,       /* Number of indexes in the xStack array. */
        parameters,      /* Parameter passed into the task. */
        priority,        /* Priority at which the task is created. */
        buffer,          /* Array to use as the task's stack. */
        &task_internal); /* Variable to hold the task's data structure. */
  }

  StackType_t buffer[StackSize];
  StaticTask_t task_internal;
  TaskHandle_t handle;
};
}  // namespace hakkou