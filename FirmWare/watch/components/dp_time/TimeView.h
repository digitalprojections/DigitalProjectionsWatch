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
           *sync = nullptr, *zoneLabel = nullptr, *zonePanel = nullptr;
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
  void dismissZones() {
    if (zonePanel)
      lv_obj_delete(zonePanel);
    zonePanel = nullptr;
  }
  static void dismiss(lv_event_t *e) {
    static_cast<TimeView *>(lv_event_get_user_data(e))->dismissZones();
  }
  static void choose(lv_event_t *e) {
    auto *self = static_cast<TimeView *>(lv_event_get_user_data(e));
    unsigned index = static_cast<unsigned>(reinterpret_cast<uintptr_t>(
        lv_obj_get_user_data(lv_event_get_target_obj(e))));
    self->selectZone(index);
    self->dismissZones();
  }
  static void showZones(lv_event_t *e) {
    auto *self = static_cast<TimeView *>(lv_event_get_user_data(e));
    if (self->zonePanel)
      return;
    auto *panel = self->zonePanel = lv_obj_create(self->root);
    lv_obj_remove_style_all(panel);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(panel, lv_obj_get_width(self->root), lv_obj_get_height(self->root));
    lv_obj_set_pos(panel, -12, -12);
    lv_obj_set_style_bg_color(panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(panel, 12, 0);
    lv_obj_set_style_pad_left(panel, 24, 0);
    lv_obj_set_style_pad_right(panel, 24, 0);
    lv_obj_set_style_pad_bottom(panel, 28, 0);
    lv_obj_set_style_pad_row(panel, 8, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    auto *back = lv_button_create(panel);
    lv_obj_set_size(back, LV_PCT(100), 48);
    auto *title = lv_label_create(back);
    lv_label_set_text(title, LV_SYMBOL_LEFT "  Time zones");
    lv_obj_center(title);
    lv_obj_add_event_cb(back, dismiss, LV_EVENT_CLICKED, self);
    auto *list = lv_obj_create(panel);
    lv_obj_remove_style_all(list);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    for (unsigned i = 0; i < timeZoneCount; ++i) {
      auto *row = lv_button_create(list);
      lv_obj_set_size(row, LV_PCT(100), 54);
      lv_obj_set_user_data(row,
                           reinterpret_cast<void *>(static_cast<uintptr_t>(i)));
      auto *label = lv_label_create(row);
      lv_label_set_text(label, timeZones[i].name);
      lv_obj_center(label);
      lv_obj_add_event_cb(row, choose, LV_EVENT_CLICKED, self);
    }
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
    picker = lv_button_create(root);
    lv_obj_set_size(picker, 330, 48);
    zoneLabel = lv_label_create(picker);
    lv_obj_center(zoneLabel);
    lv_obj_add_event_cb(picker, showZones, LV_EVENT_CLICKED, this);
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
    lv_label_set_text(zoneLabel,
                      timeZones[d.zone < timeZoneCount ? d.zone : 0].name);
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
      lv_label_set_text(zoneLabel, timeZones[index].name);
      if (onZone)
        onZone(user, index);
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
    zonePanel = nullptr;
    canSync = false;
  }
};
} // namespace dpwatch
