#pragma once
#include "PhoneSetupView.h"
namespace esp_brookesia::apps {
class BluetoothPage {
  lv_obj_t *root = nullptr;
  lv_timer_t *timer = nullptr;
  dpwatch::PhoneSetupView view;
  dpwatch::PhoneSetupSnapshot last{};
  static void refresh(lv_timer_t *timer);

public:
  static BluetoothPage &instance() {
    static BluetoothPage page;
    return page;
  }
  void run();
  void close();
};
} // namespace esp_brookesia::apps
