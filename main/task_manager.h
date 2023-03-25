#pragma once

#include "defines.h"
#include "platform/platform.h"
namespace hakkou {

// template <typename T>
// using FreeRTOSPtr =
//     std::unique_ptr<T, decltype([](T* ptr) { vPortFree(ptr); })>;

void task_manager(void*) {
  UBaseType_t num_tasks = 0;
  //   FreeRTOSPtr<TaskStatus_t> task_array;

  constexpr u8 line_length = 100 + 1;
  constexpr u8 max_tasks = 100;

  // temp variables for computing task stats
  u32 total_runtime;
  u32 task_runtime_percentage;

  // allocate the string buffer
  char* buf =
      static_cast<char*>(malloc(line_length * max_tasks * sizeof(char)));
  sprintf(buf, "%-15s     %10s     %4s     %8s     %5s\n", "Task Name",
          "Stack HM", "Priority", "Core", "%Time");
  sprintf(buf + line_length,
          "==============================================================\n");

  UBaseType_t x;
  unsigned long ulTotalRunTime, ulStatsAsPercentage;
  while (true) {
    num_tasks = uxTaskGetNumberOfTasks();

    std::vector<TaskStatus_t> tasks;
    tasks.resize(num_tasks);

    num_tasks = uxTaskGetSystemState(tasks.data(), num_tasks, &ulTotalRunTime);

    char* ptr = buf + 2 * line_length;
    if (num_tasks == 0) {
      // memroy error fuck out of here
      // todo handle better :D
      vTaskDelete(NULL);
    }

    /* For percentage calculations. */
    ulTotalRunTime /= 100UL;

    if (ulTotalRunTime == 0) {
      HWARN("Run time is 0");
      /* Avoid divide by zero errors. */
      platform_sleep(1000);
      continue;
    }

    std::sort(tasks.begin(), tasks.end(),
              [](const TaskStatus_t& ta, TaskStatus_t& tb) {
                return ta.usStackHighWaterMark > tb.usStackHighWaterMark;
              });
    /* For each populated position in the pxTaskStatusArray array,
    format the raw data as human readable ASCII data. */
    for (const auto& task : tasks) {
      /* What percentage of the total run time has the task used?
      This will always be rounded down to the nearest integer.
      ulTotalRunTimeDiv100 has already been divided by 100. */
      ulStatsAsPercentage = task.ulRunTimeCounter / ulTotalRunTime;

      char core_affinity[15];
      if (task.xCoreID == tskNO_AFFINITY) {
        core_affinity[0] = '~';
        core_affinity[1] = '\0';
      } else {
        sprintf(core_affinity, "%d", task.xCoreID);
      }
      sprintf(ptr, "%-15s     %10ld     %5d/%2d     %4s     %4ld%%\n",
              task.pcTaskName, task.usStackHighWaterMark,
              task.uxCurrentPriority, task.uxBasePriority, core_affinity,
              ulStatsAsPercentage);

      ptr = ptr + line_length;
    }

    // add a bottom line
    sprintf(ptr,
            "==============================================================\n");

    ptr = buf;
    for (x = 0; x < num_tasks + 3; x++) {
      printf("%s", ptr);
      ptr = ptr + line_length;
    }

    platform_sleep(5000);
  }
}

}  // namespace hakkou