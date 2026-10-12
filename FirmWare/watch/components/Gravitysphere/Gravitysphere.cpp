/*
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Shared calibration and rectangular layout: Digital Projections.
 */
#include "Gravitysphere.hpp"
#include "esp_lib_utils.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
LV_IMG_DECLARE(img_app_qmi8658ball);
namespace esp_brookesia::apps {
Gravitysphere::Gravitysphere(bool status, bool navigation)
    : App("GravitySphere", &img_app_qmi8658ball, true, status, navigation) {}
Gravitysphere *Gravitysphere::requestInstance(bool status, bool navigation) {
    static Gravitysphere instance(status, navigation);
    return &instance;
}
bool Gravitysphere::run() {
    root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0x101827), 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    label = lv_label_create(root);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, lv_color_hex(0xE4DDCE), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 8);
    auto *guide = lv_label_create(root);
    lv_label_set_text(guide, "BOOT: pause / resume\nDouble-press: calibrate | Hold: centre");
    lv_obj_set_style_text_color(guide, lv_color_hex(0xA9BBCF), 0);
    lv_obj_set_style_text_align(guide, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(guide, LV_ALIGN_BOTTOM_MID, 0, -4);
    ball = lv_obj_create(root);
    lv_obj_set_size(ball, 44, 44);
    lv_obj_set_style_radius(ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ball, lv_color_hex(0x52D6BE), 0);
    lv_obj_set_style_border_width(ball, 0, 0);
    lv_obj_remove_flag(ball, LV_OBJ_FLAG_SCROLLABLE);
    button = dpwatch::BootButton{}; x = y = 0; paused = false;
    lastTick = lv_tick_get();
    timer = lv_timer_create(poll, 20, this);
    return timer != nullptr;
}
void Gravitysphere::poll(lv_timer_t *timer) {
    auto *self = static_cast<Gravitysphere *>(lv_timer_get_user_data(timer));
    const auto data = dpwatch::motionSnapshot();
    const unsigned now = lv_tick_get();
    const float seconds = std::min(now-self->lastTick, 50u) / 1000.0f;
    self->lastTick = now;
    switch (self->button.update(data.bootPressed, now)) {
    case dpwatch::ButtonAction::Calibrate: dpwatch::requestMotionCalibration(); break;
    case dpwatch::ButtonAction::TogglePause: self->paused = !self->paused; break;
    case dpwatch::ButtonAction::NewGame: self->x = self->y = 0; break;
    default: break;
    }
    const char *text = "Double-press BOOT to calibrate";
    switch (data.state) {
    case dpwatch::CalibrationState::Loading: text = "Keep flat and still - calibrating"; break;
    case dpwatch::CalibrationState::Ready: text = self->paused ? "Paused" : "Tilt to move"; break;
    case dpwatch::CalibrationState::Unavailable: text = "Motion sensor unavailable"; break;
    case dpwatch::CalibrationState::Error: text = "Calibration failed - double-press BOOT"; break;
    default: break;
    }
    if (lv_label_get_text(self->label) != std::string(text)) lv_label_set_text(self->label, text);
    if (!self->paused && data.state == dpwatch::CalibrationState::Ready && data.ageMs < 100) {
        const float horizontal = std::fabs(data.sample.ay) < 0.025f ? 0.0f : data.sample.ay;
        const float vertical = std::fabs(data.sample.ax) < 0.025f ? 0.0f : data.sample.ax;
        self->x -= horizontal * 240 * seconds;
        self->y += vertical * 240 * seconds;
    }
    const float limitX = std::max(0.0f, (lv_obj_get_content_width(self->root)-44)/2.0f);
    const float limitY = std::max(0.0f, (lv_obj_get_content_height(self->root)-160)/2.0f);
    self->x = std::clamp(self->x, -limitX, limitX);
    self->y = std::clamp(self->y, -limitY, limitY);
    lv_obj_align(self->ball, LV_ALIGN_CENTER, static_cast<int>(self->x), static_cast<int>(self->y));
}
bool Gravitysphere::pause() { if (timer) lv_timer_pause(timer); return true; }
bool Gravitysphere::resume() { lastTick=lv_tick_get(); button=dpwatch::BootButton{}; if (timer) lv_timer_resume(timer); return true; }
bool Gravitysphere::close() {
    if (timer) { lv_timer_delete(timer); timer=nullptr; }
    if (root) { lv_obj_delete(root); root=nullptr; }
    return true;
}
ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, Gravitysphere, "GravitySphere", []() {
    return std::shared_ptr<Gravitysphere>(Gravitysphere::requestInstance(), [](Gravitysphere *) {});
})
}
