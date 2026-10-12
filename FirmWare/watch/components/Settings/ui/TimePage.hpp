#pragma once
#include "TimeView.h"
namespace esp_brookesia::apps {
class TimePage {
  lv_obj_t *root = nullptr;
  lv_timer_t *timer = nullptr;
  dpwatch::TimeView view;
  bool saveFailed = false;
  static void refresh(lv_timer_t *timer);
  void update();

public:
  static TimePage &instance() {
    static TimePage page;
    return page;
  }
  void run();
  void close();
  void selectZone(unsigned index) { view.selectZone(index); }
  void syncNow() { view.syncNow(); }
};
} // namespace esp_brookesia::apps
