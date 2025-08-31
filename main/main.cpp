
#include "defines.h"
#include "event.h"
#include "fsm.h"
#include "internal_types.h"
#include "logger.h"
#include "ota.h"
#include "platform/platform.h"

///////TODO: Temp includes

#include "hardware/lcd.h"

#include <hardware/encoder.h>

#include "soc/gpio_num.h"
#include "task_manager.h"

#include "services/ble/ble.h"
#include "services/http_server.h"
// #include "services/rest.h"
#include "services/wifi.h"
//////////

#include "esp_chip_info.h"    // TODO DELETE
#include "esp_http_server.h"  // TODO DELETE
#include "esp_log.h"

#include <rom/ets_sys.h>  // us delay
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "services/ble/common.h"
#include "services/ble/gap.h"
#include "services/ble/gatt_svc.h"

using namespace hakkou;

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  // cJSON* root = cJSON_CreateObject();
  // esp_chip_info_t chip_info;
  // esp_chip_info(&chip_info);
  // cJSON_AddStringToObject(root, "version", IDF_VER);
  // cJSON_AddNumberToObject(root, "cores", chip_info.cores);
  // const char* sys_info = cJSON_Print(root);
  const char* sys = "{\"as\": 2}";
  httpd_resp_sendstr(req, sys);
  // free((void*)sys_info);
  // cJSON_Delete(root);
  return ESP_OK;
}

void setup_base_hardware() {
  // temporary bring up routine
  LCD* lcd = new LCD();

  xTaskCreate(rotary_encoder_task, "RotaryEncoder",
              configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}


void setup_sensors() {

}

extern "C" void app_main(void) {
  // Handle any unexpected software restarts from before
  // We use this to ensure a sane state of the system
  // E.g., important GPIO to be in a known safe state.
  // TODO: handle restart

  // First get important hardware things setup
  platform_initialize({
      .interrupts_enabled = true,
      .i2c_enabled = true,
  });

  // Initialize various subsystems
  // Note(DBA): If this were a normal application, we'd
  // also ensure to release the allocated memory etc.
  // However, most are equivalent require the whole time,
  // so memory release would be equivalent to system shutdown
  // or
  initialize_logging();
  if (!event_initialize()) {
    HERROR("Couldn't initialize the event subsystem. Halting!");
    vTaskSuspend(NULL);  // TODO: Should probably restart into error mode
  }

  // depends on event system
  setup_base_hardware();

  // bring up wifi
  http_server_init();

  ESP_ERROR_CHECK(
      http_server_register_uri_handler({.uri = "/api/v1/system/info",
                                        .method = HTTP_GET,
                                        .handler = system_info_get_handler,
                                        .user_ctx = nullptr}));

  wifi_initialize();


  TaskHandle_t xHandle = NULL;
  xTaskCreate(task_manager, "TaskManager", 4192, nullptr, 20, &xHandle);
  platform_sleep(2000);

  MainMenuFSM* fsm = new MainMenuFSM();
  event_post(Event{
      .event_type = EventType::System,
      .system_event = SystemEventStart{},
  });

  // Now start the main FSM
  fsm->run();

  vTaskSuspend(NULL);
}
