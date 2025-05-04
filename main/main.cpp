
#include "defines.h"
#include "event.h"
#include "fsm.h"
#include "internal_types.h"
#include "logger.h"
#include "platform/platform.h"

///////TODO: Temp includes
#include "controller.h"
#include "hardware/bme280.h"
#include "hardware/fan.h"
#include "hardware/h_ds18x20.h"
#include "hardware/heater.h"
#include "hardware/lcd.h"

// #include "hardware/nec_remote.h"

#include "task_manager.h"

#include "services/ble/ble.h"
#include "services/http_server.h"
// #include "services/rest.h"
#include "services/wifi.h"
//////////

#include "esp_chip_info.h"    // TODO DELETE
#include "esp_http_server.h"  // TODO DELETE
#include "esp_log.h"

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

/* Library function declarations */

#ifdef __cplusplus
extern "C" {
#endif

void ble_store_config_init(void);

#ifdef __cplusplus
}
#endif

/* Private function declarations */
static void nimble_host_config_init(void);
static void nimble_host_task(void* param);

/* Private functions */
/*
 *  Stack event callback functions
 *      - on_stack_reset is called when host resets BLE stack due to errors
 *      - on_stack_sync is called when host has synced with controller
 */
static void on_stack_reset(int reason) {
  /* On reset, print reset reason to console */
  ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
  /* On stack sync, do advertising initialization */
  adv_init();
}

static void nimble_host_config_init(void) {
  /* Set host callbacks */
  ble_hs_cfg.reset_cb = on_stack_reset;
  ble_hs_cfg.sync_cb = on_stack_sync;
  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  /* Security manager configuration */
  ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_ONLY;
  ble_hs_cfg.sm_bonding = 1;
  ble_hs_cfg.sm_mitm = 1;
  ble_hs_cfg.sm_our_key_dist |=
      BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
  ble_hs_cfg.sm_their_key_dist |=
      BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

  /* Store host configuration */
  ble_store_config_init();
}

static void nimble_host_task(void* param) {
  /* Task entry log */
  ESP_LOGI(TAG, "nimble host task has been started!");

  /* This function won't return until nimble_port_stop() is executed */
  nimble_port_run();

  /* Clean up at exit */
  vTaskDelete(NULL);
}

void init_ble(void) {
  /* Local variables */
  int rc;
  uint32_t seed = esp_random();
  esp_err_t ret;

  /* Random generator initialization */
  srand(seed);

  /* NimBLE stack initialization */
  ret = nimble_port_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ", ret);
    return;
  }

  // /* GAP service initialization */
  rc = gap_init();
  if (rc != 0) {
    ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
    return;
  }

  /* GATT server initialization */
  rc = gatt_svc_init();
  if (rc != 0) {
    ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
    return;
  }

  /* NimBLE host configuration initialization */
  nimble_host_config_init();

  // /* Start NimBLE host task thread and return */
  xTaskCreate(nimble_host_task, "NimBLE Host", 4 * 1024, NULL, 5, NULL);
}

void setup_hardware() {
  // temporary bring up routine
  LCD* lcd = new LCD();

  // auto remote = new NECRemote(CONFIG_IR_ADDR);
  // if (!event_register(EventType::IRCode, nullptr, map_ircode_to_gui_cmd)) {
  //   HERROR("Couldn't initialize the IR remote!");
  // }

  Fan4W* fan = new Fan4W(CONFIG_FAN_PWM_PIN, CONFIG_FAN_TACHO_PIN);
  event_post(
      {.event_type = EventType::FanDuty, .sender = nullptr, .fan_duty = 30});

  BME280* bme = new BME280(static_cast<i2c_port_t>(CONFIG_BMP280_I2C_ADDRESS),
                           static_cast<gpio_num_t>(CONFIG_I2C_SDA_PIN),
                           static_cast<gpio_num_t>(CONFIG_I2C_SCL_PIN), 2000);

  DS18X20* ds18x20 =
      new DS18X20(static_cast<gpio_num_t>(CONFIG_ONEWIRE_PIN), 2000);

  Heater* heater = new Heater(CONFIG_HEATER_PWM_PIN);
}

void setup_hardware_debug() {
  LCD* lcd = new LCD();

  // todo: create a fake food/amb temp setup
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

  // bring up wifi
  http_server_init();

  ESP_ERROR_CHECK(http_server_register_uri_handler({.uri = "/api/v1/system/info",
    .method = HTTP_GET,
    .handler = system_info_get_handler,
    .user_ctx = nullptr}));

  wifi_initialize();
  init_ble();
  // rest_initialize();
  // setup_hardware_debug();

  TaskHandle_t xHandle = NULL;
  xTaskCreate(task_manager, "TaskManager", 2048, nullptr, 20, &xHandle);
  platform_sleep(2000);

  // auto ctrl = new Controller();
  // // wait for sensors to initialzie...
  // ctrl->run();

  // MainMenuFSM* fsm = new MainMenuFSM();
  // event_post(Event{
  //     .event_type = EventType::System,
  //     .system_event = SystemEventStart{},
  // });

  // // Now start the main FSM
  // fsm->run();

  vTaskSuspend(NULL);
}
