#pragma once
#include "logger.h"
#include "platform/platform.h"

#include "ext/nec_ir_remote.h"

#include "freertos/FreeRTOS.h"

#include <functional>

/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "driver/rmt_rx.h"
#include "esp_log.h"
#include "ext/nec_ir_remote.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define EXAMPLE_IR_RESOLUTION_HZ 1000000  // 1MHz resolution, 1 tick = 1us
#define EXAMPLE_IR_TX_GPIO_NUM 18
// #define static_cast <gpio_num_t>(CONFIG_IR_PIN) 19
#define EXAMPLE_IR_NEC_DECODE_MARGIN \
  200  // Tolerance for parsing RMT symbols into bit stream

/**
 * @brief NEC timing spec
 */
#define NEC_LEADING_CODE_DURATION_0 9000
#define NEC_LEADING_CODE_DURATION_1 4500
#define NEC_PAYLOAD_ZERO_DURATION_0 560
#define NEC_PAYLOAD_ZERO_DURATION_1 560
#define NEC_PAYLOAD_ONE_DURATION_0 560
#define NEC_PAYLOAD_ONE_DURATION_1 1690
#define NEC_REPEAT_CODE_DURATION_0 9000
#define NEC_REPEAT_CODE_DURATION_1 2250

/**
 * @brief Saving NEC decode results
 */
static uint16_t s_nec_code_address;
static uint16_t s_nec_code_command;

/**
 * @brief Check whether a duration is within expected range
 */
static inline bool nec_check_in_range(uint32_t signal_duration,
                                      uint32_t spec_duration) {
  return (signal_duration < (spec_duration + EXAMPLE_IR_NEC_DECODE_MARGIN)) &&
         (signal_duration > (spec_duration - EXAMPLE_IR_NEC_DECODE_MARGIN));
}

/**
 * @brief Check whether a RMT symbol represents NEC logic zero
 */
static bool nec_parse_logic0(rmt_symbol_word_t* rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_PAYLOAD_ZERO_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_PAYLOAD_ZERO_DURATION_1);
}

/**
 * @brief Check whether a RMT symbol represents NEC logic one
 */
static bool nec_parse_logic1(rmt_symbol_word_t* rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_PAYLOAD_ONE_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_PAYLOAD_ONE_DURATION_1);
}

/**
 * @brief Decode RMT symbols into NEC address and command
 */
static bool nec_parse_frame(rmt_symbol_word_t* rmt_nec_symbols) {
  rmt_symbol_word_t* cur = rmt_nec_symbols;
  uint16_t address = 0;
  uint16_t command = 0;
  bool valid_leading_code =
      nec_check_in_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) &&
      nec_check_in_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);
  if (!valid_leading_code) {
    return false;
  }
  cur++;
  for (int i = 0; i < 16; i++) {
    if (nec_parse_logic1(cur)) {
      address |= 1 << i;
    } else if (nec_parse_logic0(cur)) {
      address &= ~(1 << i);
    } else {
      return false;
    }
    cur++;
  }
  for (int i = 0; i < 16; i++) {
    if (nec_parse_logic1(cur)) {
      command |= 1 << i;
    } else if (nec_parse_logic0(cur)) {
      command &= ~(1 << i);
    } else {
      return false;
    }
    cur++;
  }
  // save address and command
  s_nec_code_address = address;
  s_nec_code_command = command;
  return true;
}

/**
 * @brief Check whether the RMT symbols represent NEC repeat code
 */
static bool nec_parse_frame_repeat(rmt_symbol_word_t* rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_REPEAT_CODE_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_REPEAT_CODE_DURATION_1);
}

/**
 * @brief Decode RMT symbols into NEC scan code and print the result
 */
static void example_parse_nec_frame(rmt_symbol_word_t* rmt_nec_symbols,
                                    size_t symbol_num) {
  printf("NEC frame start---\r\n");
  for (size_t i = 0; i < symbol_num; i++) {
    printf("{%d:%d},{%d:%d}\r\n", rmt_nec_symbols[i].level0,
           rmt_nec_symbols[i].duration0, rmt_nec_symbols[i].level1,
           rmt_nec_symbols[i].duration1);
  }
  printf("---NEC frame end: ");
  // decode RMT symbols
  switch (symbol_num) {
    case 34:  // NEC normal frame
      if (nec_parse_frame(rmt_nec_symbols)) {
        printf("Address=%04X, Command=%04X\r\n\r\n", s_nec_code_address,
               s_nec_code_command);
      }
      break;
    case 2:  // NEC repeat frame
      if (nec_parse_frame_repeat(rmt_nec_symbols)) {
        printf("Address=%04X, Command=%04X, repeat\r\n\r\n", s_nec_code_address,
               s_nec_code_command);
      }
      break;
    default:
      printf("Unknown NEC frame\r\n\r\n");
      break;
  }
}

static bool example_rmt_rx_done_callback(rmt_channel_handle_t channel,
                                         const rmt_rx_done_event_data_t* edata,
                                         void* user_data) {
  BaseType_t high_task_wakeup = pdFALSE;
  QueueHandle_t receive_queue = (QueueHandle_t)user_data;
  // send the received RMT symbols to the parser task
  xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
  return high_task_wakeup == pdTRUE;
}

namespace hakkou {

class NECRemote {
 public:
  // constexpr static rmt_channel_t RX_CHANNEL =
  //     rmt_channel_t(CONFIG_IR_RMT_CHANNEL);
  constexpr static gpio_num_t RX_PIN = static_cast<gpio_num_t>(CONFIG_IR_PIN);

  NECRemote(uint32_t address, bool skip_repeats)
      : address_(address), skip_repeats_(skip_repeats) {
    xTaskCreate(initialize, "NEC_IR", 4196, this, 10, nullptr);
  }

  // ~NECRemote() {}

 private:
  static void initialize(void* cls) { static_cast<NECRemote*>(cls)->run(); }
  void run() {
    HINFO("create RMT RX channel");
    rmt_rx_channel_config_t rx_channel_cfg = {
        .gpio_num = static_cast<gpio_num_t>(CONFIG_IR_PIN),
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = EXAMPLE_IR_RESOLUTION_HZ,
        .mem_block_symbols =
            64,  // amount of RMT symbols that the channel can store at a time
    };
    rmt_channel_handle_t rx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &rx_channel));

    HINFO("register RX done callback");
    QueueHandle_t receive_queue =
        xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    assert(receive_queue);
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = example_rmt_rx_done_callback,
    };
    ESP_ERROR_CHECK(
        rmt_rx_register_event_callbacks(rx_channel, &cbs, receive_queue));

    // the following timing requirement is based on NEC protocol
    rmt_receive_config_t receive_config = {
        .signal_range_min_ns =
            1250,  // the shortest duration for NEC signal is 560us, 1250ns <
                   // 560us, valid signal won't be treated as noise
        .signal_range_max_ns =
            12000000,  // the longest duration for NEC signal is 9000us,
                       // 12000000ns > 9000us, the receive won't stop early
    };

    HINFO("install IR NEC encoder");
    ir_nec_encoder_config_t nec_encoder_cfg = {
        .resolution = EXAMPLE_IR_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t nec_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_ir_nec_encoder(&nec_encoder_cfg, &nec_encoder));

    HINFO("enable RMT TX and RX channels");
    // ESP_ERROR_CHECK(rmt_enable(tx_channel));
    ESP_ERROR_CHECK(rmt_enable(rx_channel));

    // save the received RMT symbols
    rmt_symbol_word_t raw_symbols[64];  // 64 symbols should be sufficient for a
                                        // standard NEC frame
    rmt_rx_done_event_data_t rx_data;
    // ready to receive
    ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols),
                                &receive_config));
    while (1) {
      // wait for RX done signal
      if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(1000)) ==
          pdPASS) {
        // parse the receive symbols and print the result
        example_parse_nec_frame(rx_data.received_symbols, rx_data.num_symbols);
        // start receive again
        ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols,
                                    sizeof(raw_symbols), &receive_config));
      }
    }

    vTaskDelete(NULL);
  }

  uint32_t address_;
  bool skip_repeats_;
};

}  // namespace hakkou