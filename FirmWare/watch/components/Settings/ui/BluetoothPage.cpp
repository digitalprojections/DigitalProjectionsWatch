#include "BluetoothPage.hpp"
#include "../Settings.hpp"
#include "SettingsUI.hpp"
#include <cstring>
namespace esp_brookesia::apps {
void BluetoothPage::run() {
  close();
  root = settings_ui::create_page(lv_screen_active());
  settings_ui::create_header(root, "Bluetooth", [](lv_event_t *) {
    lv_async_call([](void *) { Settings::requestInstance()->showRootPage(); },
                  nullptr);
  });
  auto *body = lv_obj_create(root);
  lv_obj_remove_style_all(body);
  lv_obj_set_width(body, lv_pct(100));
  lv_obj_update_layout(root);
  lv_obj_set_height(body,
                    lv_obj_get_height(root) - settings_ui::PAGE_HEADER_HEIGHT);
  lv_obj_align(body, LV_ALIGN_BOTTOM_MID, 0, 0);
  view.open(
      body, [] { dpwatch::startPhoneSetup(); },
      [] { dpwatch::stopPhoneSetup(); });
  last = dpwatch::phoneSetupSnapshot();
  view.update(last);
  timer = lv_timer_create(refresh, 250, this);
}
void BluetoothPage::refresh(lv_timer_t *timer) {
  auto *self = static_cast<BluetoothPage *>(lv_timer_get_user_data(timer));
  const auto state = dpwatch::phoneSetupSnapshot();
  if (state.state != self->last.state ||
      state.canStart != self->last.canStart ||
      state.canStop != self->last.canStop ||
      std::strcmp(state.code, self->last.code) != 0) {
    self->view.update(state);
    self->last = state;
  }
}
void BluetoothPage::close() {
  if (timer) {
    lv_timer_delete(timer);
    timer = nullptr;
  }
  if (root) {
    dpwatch::stopPhoneSetup();
    view.close();
    lv_obj_delete(root);
    root = nullptr;
  }
}
} // namespace esp_brookesia::apps
