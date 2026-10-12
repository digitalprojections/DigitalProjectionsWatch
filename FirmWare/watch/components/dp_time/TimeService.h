#pragma once
#include "TimeModel.h"
#include "esp_err.h"
#include <ctime>
namespace dpwatch {
struct TimeSnapshot {
  TimeState state = TimeState::WaitingWifi;
  TimeSource source = TimeSource::None;
  bool valid = false, online = false, rtcAvailable = false, rtcSaved = false;
  unsigned zone = 0;
  uint64_t epoch = 0, lastSync = 0, rtcEpoch = 0;
  esp_err_t error = ESP_OK, rtcError = ESP_OK;
};
// Poll runs on the existing resident status worker; no additional task stack.
esp_err_t startTimeService();
void pollTimeService(bool connected);
void requestTimeSync();
esp_err_t setTimeZone(unsigned index);
TimeSnapshot timeSnapshot();
} // namespace dpwatch
