#pragma once
#include "TimeModel.h"
#include "lvgl.h"
#include <string>
namespace dpwatch {
struct TimeViewData {
  TimeState state = TimeState::WaitingWifi;
  bool online = false, valid = false;
  unsigned zone = 0;
  const char *clock = "--:--:--";
  const char *date = "Date unavailable";
  const char *detail = "Connect Wi-Fi to synchronize";
  const char *error = nullptr;
};
class TimeView {
  lv_obj_t *root = nullptr, *clock = nullptr, *date = nullptr,
           *picker = nullptr, *status = nullptr, *detail = nullptr,
           *sync = nullptr;
  bool canSync = false;
  void (*onSync)(void *) = nullptr;
  void (*onZone)(void *, unsigned) = nullptr;
  void *user = nullptr;
  static void clicked(lv_event_t *event) {
    auto *self = static_cast<TimeView *>(lv_event_get_user_data(event));
    if (self->canSync && self->onSync) {
      self->canSync = false;
      lv_obj_add_state(self->sync, LV_STATE_DISABLED);
      lv_label_set_text(self->status, "Synchronizing...");
      self->onSync(self->user);
    }
  }
  static void selected(lv_event_t *event) {
    auto *self = static_cast<TimeView *>(lv_event_get_user_data(event));
    if (self->onZone)
      self->onZone(self->user, lv_dropdown_get_selected(self->picker));
  }

public:
  void open(lv_obj_t *parent, void (*syncCb)(void *),
            void (*zoneCb)(void *, unsigned), void *data) {
    onSync = syncCb;
    onZone = zoneCb;
    user = data;
    root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(root, lv_color_white(), 0);
    lv_obj_set_style_text_font(root, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_all(root, 12, 0);
    lv_obj_set_style_pad_row(root, 8, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    auto label = [&](const char *text) {
      auto *l = lv_label_create(root);
      lv_label_set_text(l, text);
      lv_obj_set_width(l, 330);
      lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
      return l;
    };
    clock = label("--:--:--");
    lv_obj_set_style_text_font(clock, &lv_font_montserrat_32, 0);
    date = label("Date unavailable");
    label("Time zone");
    picker = lv_dropdown_create(root);
    lv_obj_set_size(picker, 330, 48);
    std::string options;
    for (unsigned i = 0; i < timeZoneCount; ++i) {
      if (i)
        options += '\n';
      options += timeZones[i].name;
    }
    lv_dropdown_set_options(picker, options.c_str());
    lv_obj_add_event_cb(picker, selected, LV_EVENT_VALUE_CHANGED, this);
    status = label("Waiting for Wi-Fi");
    lv_obj_set_style_text_font(status, &lv_font_montserrat_18, 0);
    detail = label("Connect Wi-Fi to synchronize");
    lv_obj_set_style_text_font(detail, &lv_font_montserrat_16, 0);
    sync = lv_button_create(root);
    lv_obj_set_size(sync, 300, 54);
    auto *l = lv_label_create(sync);
    lv_label_set_text(l, "Sync now");
    lv_obj_center(l);
    lv_obj_add_event_cb(sync, clicked, LV_EVENT_CLICKED, this);
    update(TimeViewData{});
  }
  void update(const TimeViewData &d) {
    lv_label_set_text(clock, d.clock);
    lv_label_set_text(date, d.date);
    lv_label_set_text(detail, d.detail);
    lv_dropdown_set_selected(picker, d.zone);
    const char *text = "Waiting for Wi-Fi";
    switch (d.state) {
    case TimeState::Loading:
      text = "Synchronizing...";
      break;
    case TimeState::Success:
      text = "Internet time synchronized";
      break;
    case TimeState::Error:
      text = "Sync failed. Retrying automatically.";
      break;
    case TimeState::Unavailable:
      text = "Time sync unavailable";
      break;
    case TimeState::WaitingWifi:
      if (d.valid)
        text = "Offline. Clock continues locally.";
      break;
    }
    lv_label_set_text(status, d.error ? d.error : text);
    canSync = d.online && d.state != TimeState::Loading &&
              d.state != TimeState::Unavailable;
    if (canSync)
      lv_obj_remove_state(sync, LV_STATE_DISABLED);
    else
      lv_obj_add_state(sync, LV_STATE_DISABLED);
  }
  void selectZone(unsigned index) {
    if (root && picker && index < timeZoneCount) {
      lv_dropdown_set_selected(picker, index);
      lv_obj_send_event(picker, LV_EVENT_VALUE_CHANGED, nullptr);
    }
  }
  void syncNow() {
    if (root && sync)
      lv_obj_send_event(sync, LV_EVENT_CLICKED, nullptr);
  }
  void close() {
    if (root)
      lv_obj_delete(root);
    root = nullptr;
    canSync = false;
  }
};
} // namespace dpwatch
