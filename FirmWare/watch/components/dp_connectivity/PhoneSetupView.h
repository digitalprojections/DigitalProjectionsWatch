#pragma once
#include "PhoneSetup.h"
#include <cstring>
#include <lvgl.h>
namespace dpwatch {
class PhoneSetupView {
  lv_obj_t *root = nullptr, *status = nullptr, *details = nullptr,
           *button = nullptr, *buttonText = nullptr;
  PhoneSetupSnapshot current{};
  void (*start)() = nullptr;
  void (*stop)() = nullptr;
  static void clicked(lv_event_t *e) {
    auto *self = static_cast<PhoneSetupView *>(lv_event_get_user_data(e));
    if (self->current.canStart && self->start)
      self->start();
    else if (self->current.canStop && self->stop)
      self->stop();
  }

public:
  void open(lv_obj_t *parent, void (*onStart)(), void (*onStop)()) {
    start = onStart;
    stop = onStop;
    root = lv_obj_create(parent);
    lv_obj_set_size(root, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0x101827), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 16, 0);
    lv_obj_set_style_text_font(root, &lv_font_montserrat_20, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root, 14, 0);
    auto *intro = lv_label_create(root);
    lv_obj_set_width(intro, lv_pct(100));
    lv_label_set_text(
        intro,
        "Enter your Wi-Fi password on your phone using the ESP BLE Prov app.");
    lv_obj_set_style_text_color(intro, lv_color_hex(0xC6D2E2), 0);
    status = lv_label_create(root);
    lv_obj_set_width(status, lv_pct(100));
    lv_obj_set_style_text_color(status, lv_color_hex(0xFFFFFF), 0);
    details = lv_label_create(root);
    lv_obj_set_width(details, lv_pct(100));
    lv_obj_set_style_text_color(details, lv_color_hex(0xC6D2E2), 0);
    button = lv_button_create(root);
    lv_obj_set_size(button, lv_pct(100), 60);
    buttonText = lv_label_create(button);
    lv_obj_center(buttonText);
    lv_obj_add_event_cb(button, clicked, LV_EVENT_CLICKED, this);
    update({});
  }
  void update(const PhoneSetupSnapshot &s) {
    if (!root)
      return;
    current = s;
    constexpr const char *texts[] = {
        "Bluetooth setup is off",
        "Starting Bluetooth...",
        "Ready for your phone",
        "Connecting to Wi-Fi...",
        "Wi-Fi connected",
        "Connection failed. Check the password and retry on your phone.",
        "Stopping setup...",
        "Setup unavailable. Tap Retry to try again."};
    lv_label_set_text(status, texts[static_cast<unsigned>(s.state)]);
    if (s.canStop && s.code[0])
      lv_label_set_text_fmt(details, "Device: %s\nSetup code: %s", s.name,
                            s.code);
    else
      lv_label_set_text(
          details, s.state == SetupState::Success
                       ? "Your network is saved. Bluetooth setup has finished."
                       : "Select Start, then choose this watch in the phone "
                         "app. Use a 2.4 GHz network.");
    lv_label_set_text(buttonText, s.canStart
                                      ? (s.state == SetupState::Unavailable
                                             ? "Retry setup"
                                             : "Start phone setup")
                                      : "Cancel setup");
    if (s.canStart || s.canStop)
      lv_obj_remove_state(button, LV_STATE_DISABLED);
    else
      lv_obj_add_state(button, LV_STATE_DISABLED);
  }
  void close() {
    if (root)
      lv_obj_delete(root);
    root = nullptr;
  }
};
} // namespace dpwatch
