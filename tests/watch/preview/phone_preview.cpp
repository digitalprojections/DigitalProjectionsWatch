#include "PhoneSetupView.h"
#include <array>
#include <cassert>
#include <cstdio>
static std::array<uint16_t, 410 * 502> pixels{};
static std::array<uint16_t, 410 * 40> buffer{};
static int starts = 0, stops = 0;
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *raw) {
  auto *source = reinterpret_cast<uint16_t *>(raw);
  for (int y = a->y1; y <= a->y2; ++y)
    for (int x = a->x1; x <= a->x2; ++x)
      pixels[y * 410 + x] = *source++;
  lv_display_flush_ready(d);
}
int main(int argc, char **argv) {
  lv_init();
  auto *d = lv_display_create(410, 502);
  lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(d, flush);
  lv_display_set_buffers(d, buffer.data(), nullptr, buffer.size() * 2,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  auto *body = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(body);
  lv_obj_set_size(body, 410, 390);
  lv_obj_set_pos(body, 0, 72);
  dpwatch::PhoneSetupView view;
  view.open(
      body, [] { ++starts; }, [] { ++stops; });
  auto *root = lv_obj_get_child(body, 0);
  auto *button = lv_obj_get_child(root, 3);
  lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
  assert(starts == 1 && stops == 0);
  dpwatch::PhoneSetupSnapshot s;
  s.state = dpwatch::SetupState::Waiting;
  s.canStart = false;
  s.canStop = true;
  std::snprintf(s.name, sizeof(s.name), "PROV_DP_123456");
  std::snprintf(s.code, sizeof(s.code), "12AB34CD");
  view.update(s);
  lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
  assert(starts == 1 && stops == 1);
  s.state = dpwatch::SetupState::Stopping;
  s.canStop = false;
  view.update(s);
  assert(lv_obj_has_state(button, LV_STATE_DISABLED));
  lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
  assert(starts == 1 && stops == 1);
  s.state = dpwatch::SetupState::Success;
  s.canStart = true;
  s.code[0] = 0;
  view.update(s);
  assert(!lv_obj_has_state(button, LV_STATE_DISABLED));
  s.state = dpwatch::SetupState::Waiting;
  s.canStart = false;
  s.canStop = true;
  std::snprintf(s.code, sizeof(s.code), "12AB34CD");
  view.update(s);
  lv_obj_update_layout(root);
  assert(lv_obj_get_height(button) >= 60);
  lv_refr_now(d);
  auto *f = std::fopen(argc > 1 ? argv[1] : "phone-preview.ppm", "wb");
  if (!f)
    return 1;
  std::fprintf(f, "P6\n410 502\n255\n");
  for (auto c : pixels) {
    unsigned char rgb[] = {
        static_cast<unsigned char>(((c >> 11) & 31) * 255 / 31),
        static_cast<unsigned char>(((c >> 5) & 63) * 255 / 63),
        static_cast<unsigned char>((c & 31) * 255 / 31)};
    std::fwrite(rgb, 1, 3, f);
  }
  std::fclose(f);
  view.close();
  view.close();
  for (int i = 0; i < 30; ++i) {
    view.open(body, nullptr, nullptr);
    view.close();
    assert(lv_obj_get_child_count(body) == 0);
  }
  std::puts("Phone setup LVGL states, actions, disabled controls and 30 "
            "open/close cycles passed.");
  return 0;
}
