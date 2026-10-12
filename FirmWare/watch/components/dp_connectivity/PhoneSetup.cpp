#include "PhoneSetup.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "system_status.hpp"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include <atomic>
#include <cstdio>
#include <cstring>
namespace dpwatch {
namespace {
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
ProvisioningState model;
PhoneSetupSnapshot snapshot;
std::atomic_bool cancel{false}, ended{false}, accepted{false}, retry{false};
void publish() {
  snapshot.state = model.state();
  snapshot.canStart = model.canStart();
  snapshot.canStop = model.canStop();
}
void event(void *, esp_event_base_t, int32_t id, void *) {
  portENTER_CRITICAL(&mux);
  switch (id) {
  case WIFI_PROV_START:
    model.advertise();
    break;
  case WIFI_PROV_CRED_RECV:
    model.received();
    break;
  case WIFI_PROV_CRED_FAIL:
    model.failed();
    retry = true;
    break;
  case WIFI_PROV_CRED_SUCCESS:
    model.succeeded();
    accepted = true;
    break;
  case WIFI_PROV_END:
    ended = true;
    break;
  default:
    break;
  }
  publish();
  portEXIT_CRITICAL(&mux);
}
void worker(void *) {
  namespace status = brookesia::system_status;
  wifi_config_t previous{};
  status::Snapshot original{};
  const bool haveOriginal = status::get_snapshot(original);
  bool owns = false, initialized = false;
  esp_event_handler_instance_t handler = nullptr;
  esp_err_t error = haveOriginal ? esp_wifi_get_config(WIFI_IF_STA, &previous)
                                 : ESP_ERR_INVALID_STATE;
  if (error == ESP_OK) {
    error = status::set_wifi_provisioning(true);
    owns = error == ESP_OK;
  }
  if (error == ESP_OK)
    (void)esp_wifi_scan_stop();
  if (error == ESP_OK)
    error = esp_event_handler_instance_register(
        WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, event, nullptr, &handler);
  if (error == ESP_OK) {
    wifi_prov_mgr_config_t config{};
    config.scheme = wifi_prov_scheme_ble;
    // Do not permanently release Bluetooth memory: setup can be used again.
    config.scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE;
    config.app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE;
    config.wifi_prov_conn_cfg.wifi_conn_attempts = 3;
    error = wifi_prov_mgr_init(config);
    initialized = error == ESP_OK;
  }
  if (error == ESP_OK) {
    const auto local = phoneSetupSnapshot();
    error = wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, local.code,
                                             local.name, nullptr);
  }
  if (error == ESP_OK) {
    while (!ended.load() && !cancel.load()) {
      if (retry.exchange(false))
        wifi_prov_mgr_reset_sm_state_on_failure();
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
  // Deinit can wait on BLE tasks, so it always runs off the UI/event tasks.
  if (initialized)
    wifi_prov_mgr_deinit();
  if (handler)
    esp_event_handler_instance_unregister(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                          handler);
  if (owns) {
    if (!accepted.load()) {
      esp_wifi_disconnect();
      esp_wifi_set_storage(WIFI_STORAGE_FLASH);
      const auto restore = esp_wifi_set_config(WIFI_IF_STA, &previous);
      if (error == ESP_OK)
        error = restore;
    }
    status::set_wifi_provisioning(false);
    const auto restore =
        status::set_wifi_enabled(accepted.load() || original.wifi_enabled);
    if (error == ESP_OK)
      error = restore;
  }
  std::memset(&previous, 0, sizeof(previous));
  portENTER_CRITICAL(&mux);
  if (error != ESP_OK)
    model.unavailable();
  else
    model.finished();
  std::memset(snapshot.code, 0, sizeof(snapshot.code));
  publish();
  portEXIT_CRITICAL(&mux);
  vTaskDelete(nullptr);
}
} // namespace
PhoneSetupSnapshot phoneSetupSnapshot() {
  portENTER_CRITICAL(&mux);
  const auto result = snapshot;
  portEXIT_CRITICAL(&mux);
  return result;
}
bool startPhoneSetup() {
  uint8_t mac[6]{};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  const auto random = esp_random();
  portENTER_CRITICAL(&mux);
  if (!model.start()) {
    portEXIT_CRITICAL(&mux);
    return false;
  }
  std::snprintf(snapshot.name, sizeof(snapshot.name), "PROV_DP_%02X%02X%02X",
                mac[3], mac[4], mac[5]);
  std::snprintf(snapshot.code, sizeof(snapshot.code), "%08X",
                static_cast<unsigned>(random));
  cancel = false;
  ended = false;
  accepted = false;
  retry = false;
  publish();
  portEXIT_CRITICAL(&mux);
  if (xTaskCreate(worker, "phone_setup", 6144, nullptr, 5, nullptr) != pdPASS) {
    portENTER_CRITICAL(&mux);
    model.unavailable();
    snapshot.code[0] = 0;
    publish();
    portEXIT_CRITICAL(&mux);
    return false;
  }
  return true;
}
void stopPhoneSetup() {
  portENTER_CRITICAL(&mux);
  if (model.canStop()) {
    model.stop();
    cancel = true;
    publish();
  }
  portEXIT_CRITICAL(&mux);
}
} // namespace dpwatch
