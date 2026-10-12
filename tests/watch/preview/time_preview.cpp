#include "SettingsUI.hpp"
#include "StatusClockLabel.h"
#include "TimeView.h"
#include "WatchStatusGeometry.h"
#include <array>
#include <cassert>
#include <cstdio>
static std::array<uint16_t, 410 * 502> pixels{};
static std::array<uint16_t, 410 * 40> buffer{};
static int starts = 0;
static unsigned selectedZone = 999;
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *raw) {
  auto *source = reinterpret_cast<uint16_t *>(raw);
  for (int y = a->y1; y <= a->y2; ++y)
    for (int x = a->x1; x <= a->x2; ++x)
      pixels[y * 410 + x] = *source++;
  lv_display_flush_ready(d);
}
static lv_point_t point{};
static bool pressed = false;
static void readTouch(lv_indev_t *, lv_indev_data_t *data) {
  data->point = point;
  data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static void tap(lv_indev_t *input, lv_obj_t *object) {
  lv_area_t a;lv_obj_get_coords(object, &a);
  point = {(a.x1 + a.x2) / 2, (a.y1 + a.y2) / 2};
  pressed = true;lv_tick_inc(20);lv_indev_read(input);
  pressed = false;lv_tick_inc(20);lv_indev_read(input);
}
static void drag(lv_indev_t *input, int startY, int endY) {
  point = {205, startY}; pressed = true;lv_tick_inc(20);lv_indev_read(input);
  for (int i=1;i<=5;++i) {
    point.y = startY+(endY-startY)*i/5;lv_tick_inc(20);lv_indev_read(input);
  }
  pressed = false;lv_tick_inc(20);lv_indev_read(input);
}
int main(int argc, char **argv) {
  lv_init();
  auto *d = lv_display_create(410, 502);
  lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(d, flush);
  lv_display_set_buffers(d, buffer.data(), nullptr, buffer.size() * 2,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  auto *bar = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, 410, dpwatch::statusBarHeight);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x1a1a1a), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(bar, lv_color_white(), 0);
  auto *c = esp_brookesia::systems::phone::createStatusClockLabel(
      bar, &lv_font_montserrat_20);
  lv_label_set_text(c, "10:42");
  auto *w = lv_label_create(bar);
  lv_label_set_text(w, LV_SYMBOL_WIFI);
  lv_obj_align(w, LV_ALIGN_LEFT_MID, 40, 0);
  auto *b = lv_label_create(bar);
  lv_label_set_text(b, "85% " LV_SYMBOL_BATTERY_FULL);
  lv_obj_align(b, LV_ALIGN_RIGHT_MID, -40, 0);
  auto *app = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(app);
  lv_obj_set_size(app, 410, 454);
  lv_obj_set_pos(app, 0, 48);
  auto *page = esp_brookesia::apps::settings_ui::create_page(app);
  esp_brookesia::apps::settings_ui::create_header(page, "Date & time");
  auto *body = lv_obj_create(page);
  lv_obj_remove_style_all(body);
  lv_obj_set_size(body, 410, 382);
  lv_obj_align(body, LV_ALIGN_BOTTOM_MID, 0, 0);
  auto *input = lv_indev_create();lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input, readTouch);
  dpwatch::TimeView view;
  view.open(
      body, [](void *) { ++starts; },
      [](void *, unsigned z) { selectedZone = z; }, nullptr);
  auto *root = lv_obj_get_child(body, 0);
  auto *sync = lv_obj_get_child(root, 6);
  lv_obj_send_event(sync, LV_EVENT_CLICKED, nullptr);
  assert(starts == 0 && lv_obj_has_state(sync, LV_STATE_DISABLED));
  dpwatch::TimeViewData data;
  data.online = true;
  data.state = dpwatch::TimeState::Loading;
  view.update(data);
  lv_obj_send_event(sync, LV_EVENT_CLICKED, nullptr);
  assert(starts == 0);
  data.state = dpwatch::TimeState::Success;
  data.valid = true;
  data.clock = "10:42:08";
  data.date = "Mon, 12 Oct 2026";
  data.detail = "Source: internet\nRTC: saved";
  view.update(data);
  lv_obj_send_event(sync, LV_EVENT_CLICKED, nullptr);
  assert(starts == 1 && lv_obj_has_state(sync, LV_STATE_DISABLED));
  view.selectZone(2);
  assert(selectedZone == 2);
  view.selectZone(dpwatch::timeZoneCount);
  assert(selectedZone == 2);
  view.update(data);
  lv_obj_update_layout(page);
  lv_obj_update_layout(bar);
  auto *picker = lv_obj_get_child(root, 3);
  assert(lv_obj_check_type(picker, &lv_button_class));
  tap(input, picker);
  auto *panel = lv_obj_get_child(root, 7);
  assert(panel);
  auto *list = lv_obj_get_child(panel, 1);
  assert(lv_obj_get_child_count(list) == dpwatch::timeZoneCount);
  lv_obj_update_layout(page);
  lv_area_t first, last, viewport;
  lv_obj_get_coords(list, &viewport);
  auto *japan = lv_obj_get_child(list, 0);
  lv_obj_get_coords(japan, &first);
  assert(first.y1 >= viewport.y1 && first.y2 <= viewport.y2 && first.y1 >= 120);
  for (int i=0;i<12;++i) {drag(input,420,230);lv_obj_update_layout(page);}
  assert(lv_obj_get_scroll_y(list)>500);
  auto *brazil = lv_obj_get_child(list, dpwatch::timeZoneCount - 1);
  lv_obj_scroll_to_view(brazil, LV_ANIM_OFF);
  lv_obj_update_layout(page);
  lv_obj_get_coords(brazil, &last);
  assert(last.y1 >= viewport.y1 && last.y2 <= viewport.y2 && last.y2 < 478);
  auto beforeRefresh = lv_obj_get_scroll_y(list);
  view.update(data);
  lv_obj_update_layout(page);
  assert(lv_obj_get_scroll_y(list) == beforeRefresh);
  for (int i=0;i<12;++i) {drag(input,230,420);lv_obj_update_layout(page);}
  assert(lv_obj_get_scroll_y(list)<=0);
  lv_obj_scroll_to_view(japan, LV_ANIM_OFF);
  lv_obj_update_layout(page);
  tap(input, japan);
  assert(selectedZone == 0 && lv_obj_get_child_count(root) == 7);
  tap(input, picker);
  panel = lv_obj_get_child(root, 7);
  lv_obj_update_layout(page);
  tap(input, lv_obj_get_child(panel, 0));
  assert(selectedZone == 0 && lv_obj_get_child_count(root) == 7);

  lv_area_t a;
  lv_obj_get_coords(sync, &a);
  assert(a.y2 < 478 && lv_obj_get_height(sync) >= 54);
  for (auto *item : {c, w, b}) {
    lv_obj_get_coords(item, &a);
    assert(dpwatch::statusBoundsSafe(a.x1, a.y1, a.x2, a.y2));
  }
  tap(input, picker);
  lv_refr_now(d);
  auto *f = std::fopen(argc > 1 ? argv[1] : "time-preview.ppm", "wb");
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
  view.selectZone(0);
  view.syncNow();
  assert(selectedZone == 0 && starts == 1);
  for (int i = 0; i < 30; ++i) {
    view.open(body, nullptr, nullptr, nullptr);
    view.close();
    assert(lv_obj_get_child_count(body) == 0);
  }
  puts("Time view: offline/loading/success action gates, timezone selection, "
       "top safe bounds and 30 open-close cycles passed");
  return 0;
}
