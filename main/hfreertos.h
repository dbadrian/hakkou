#pragma once

// #define CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK 1

// If INCLUDE_vTaskSuspend is set to '1' then specifying the block time
//     as portMAX_DELAY will cause the task to block
//     indefinitely(without a timeout)
#define INCLUDE_vTaskSuspend 1


// TODO: can we more strongly type the bool values (pdTRUE/pdFALSe) etc ?

#include "freertos/FreeRTOS.h"
