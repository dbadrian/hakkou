#pragma once

#include "defines.h"
#include "platform/platform.h"
namespace hakkou {

// template <typename T>
// using FreeRTOSPtr =
//     std::unique_ptr<T, decltype([](T* ptr) { vPortFree(ptr); })>;

// TODO: Add heap information (free/allocated...fragmentation?)

void task_manager(void*) {
  UBaseType_t num_tasks = 0;
  //   FreeRTOSPtr<TaskStatus_t> task_array;

  constexpr u8 line_length = 100 + 1;
  constexpr u8 max_tasks = 100;

  // allocate the string buffer
  char* buf =
      static_cast<char*>(malloc(line_length * max_tasks * sizeof(char)));
  sprintf(buf, "%-15s     %10s     %8s     %4s     %5s\n", "Task Name",
          "Stack HM", "Priority", "Core", "%Time");
  sprintf(buf + line_length,
          "==============================================================\n");

  UBaseType_t x;
  unsigned long ulTotalRunTime, ulStatsAsPercentage;
  std::vector<TaskStatus_t> tasks;
  tasks.reserve(max_tasks - 20);

  while (true) {
    num_tasks = uxTaskGetNumberOfTasks();
    HDEBUG("[TSKMGR]: Found %d tasks.", num_tasks);
    tasks.resize(num_tasks);
    num_tasks = uxTaskGetSystemState(tasks.data(), num_tasks, &ulTotalRunTime);

    char* ptr = buf + 2 * line_length;
    if (num_tasks == 0) {
      HDEBUG("[TSKMGR]: Found no tasks, probably a memory error.");

      // memroy error fuck out of here
      // todo handle better :D
      vTaskDelete(NULL);
    }

    /* For percentage calculations. */
    ulTotalRunTime /= 100UL;
    HWARN("[TSKMGR]: %lu", ulTotalRunTime);

    if (ulTotalRunTime == 0) {
      HWARN("[TSKMGR]: Run time is 0, skipping task manager.");
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
    sprintf(ptr,
            "==============================================================\n");
    ptr = ptr + line_length;

    multi_heap_info_t heap_stats;
    heap_caps_get_info(&heap_stats, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    sprintf(ptr, "Total alloc bytes : %8u\n", heap_stats.total_allocated_bytes);
    ptr = ptr + line_length;
    sprintf(ptr, "Total free bytes  : %8u\n", heap_stats.total_free_bytes);
    ptr = ptr + line_length;
    sprintf(ptr, "Largest free block: %8u\n", heap_stats.largest_free_block);
    ptr = ptr + line_length;
    sprintf(ptr, "Free blocks : %4u\n", heap_stats.free_blocks);
    ptr = ptr + line_length;
    sprintf(ptr, "Total blocks: %4u\n", heap_stats.total_blocks);
    ptr = ptr + line_length;

    // add a bottom line
    sprintf(ptr,
            "==============================================================\n");
    ptr = ptr + line_length;

    sprintf(ptr, "Total Runtime: %fs\n",
            ulTotalRunTime * portTICK_PERIOD_MS / 100000.0);

    ptr = buf;
    for (x = 0; x < num_tasks + 10; x++) {
      platform_printf("%s", ptr);
      ptr = ptr + line_length;
    }

    platform_sleep(5000);
  }
}

}  // namespace hakkou