/* SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include "systems/phone/esp_brookesia_phone_app.hpp"
#include "BootButton.h"
#include "MotionService.h"
#include "lvgl.h"
namespace esp_brookesia::apps {
class Gravitysphere : public systems::phone::App {
public:
    static Gravitysphere *requestInstance(bool status = false, bool navigation = true);
    bool init() override { return true; }
    bool deinit() override { return close(); }
    bool run() override;
    bool back() override { return notifyCoreClosed(); }
    bool close() override;
    bool pause() override;
    bool resume() override;
private:
    Gravitysphere(bool status, bool navigation);
    static void poll(lv_timer_t *timer);
    lv_obj_t *root = nullptr, *ball = nullptr, *label = nullptr;
    lv_timer_t *timer = nullptr;
    dpwatch::BootButton button;
    dpwatch::CalibrationState displayed = dpwatch::CalibrationState::Needed;
    float x = 0, y = 0;
    bool paused = false;
    unsigned lastTick = 0;
};
}
