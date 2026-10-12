#pragma once
#include "esp_err.h"
namespace dpwatch {
// UI task / LVGL lock required for initialization and transitions.
bool startDisplayPower();
esp_err_t turnDisplayOff();
esp_err_t wakeDisplay();
bool displayConsumesBoot();
void displayPowerDiagnostic();
}
