#include "TimePage.hpp"
#include "../Settings.hpp"
#include "SettingsUI.hpp"
#include "TimeService.h"
#include <cstdio>
namespace esp_brookesia::apps {
void TimePage::run() {
  close();
  root = settings_ui::create_page(lv_screen_active());
  settings_ui::create_header(root, "Date & time", [](lv_event_t *) {
    lv_async_call([](void *) { Settings::requestInstance()->showRootPage(); },
                  nullptr);
  });
  auto *body = lv_obj_create(root);
  lv_obj_remove_style_all(body);
  lv_obj_set_width(body, LV_PCT(100));
  lv_obj_update_layout(root);
  lv_obj_set_height(body,
                    lv_obj_get_height(root) - settings_ui::PAGE_HEADER_HEIGHT);
  lv_obj_align(body, LV_ALIGN_BOTTOM_MID, 0, 0);
  view.open(
      body, [](void *) { dpwatch::requestTimeSync(); },
      [](void *data, unsigned zone) {
        auto *self = static_cast<TimePage *>(data);
        self->saveFailed = dpwatch::setTimeZone(zone) != ESP_OK;
        self->update();
      },
      this);
  timer = lv_timer_create(refresh, 1000, this);
  update();
}
void TimePage::update() {
  const auto s = dpwatch::timeSnapshot();
  dpwatch::TimeViewData d;
  d.state = s.state;
  d.online = s.online;
  d.valid = s.valid;
  d.zone = s.zone;
  char clock[16] = "--:--:--", date[32] = "Date unavailable",
       detail[100] = "Connect Wi-Fi to synchronize";
  if (s.valid) {
    time_t epoch = static_cast<time_t>(s.epoch);
    tm local{};
    localtime_r(&epoch, &local);
    strftime(clock, sizeof(clock), "%H:%M:%S", &local);
    strftime(date, sizeof(date), "%a, %d %b %Y", &local);
    snprintf(detail, sizeof(detail), "%s\nRTC: %s",
             s.source == dpwatch::TimeSource::Internet ? "Source: internet"
                                                       : "Source: saved RTC",
             s.rtcSaved       ? "saved"
             : s.rtcAvailable ? "not saved"
                              : "unavailable");
  }
  d.clock = clock;
  d.date = date;
  d.detail = detail;
  d.error = saveFailed ? "Time zone could not be saved" : nullptr;
  view.update(d);
}
void TimePage::refresh(lv_timer_t *timer) {
  static_cast<TimePage *>(lv_timer_get_user_data(timer))->update();
}
void TimePage::close() {
  if (timer) {
    lv_timer_delete(timer);
    timer = nullptr;
  }
  if (root) {
    view.close();
    lv_obj_delete(root);
    root = nullptr;
  }
  saveFailed = false;
}
} // namespace esp_brookesia::apps
