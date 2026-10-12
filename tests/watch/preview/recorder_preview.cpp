#include "RecorderLibraryView.h"
#include <array>
#include <cassert>
#include <cstdio>
static std::array<uint16_t, 410 * 502> pixels{};
static std::array<uint16_t, 410 * 40> buffer{};
static int clicks = 0;
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
  dpwatch::RecorderLibraryView view;
  view.create(
      lv_screen_active(), [](lv_event_t *) { ++clicks; }, nullptr);
  lv_obj_remove_flag(view.root, LV_OBJ_FLAG_HIDDEN);
  for (int i = 0; i < 5; ++i)
    lv_label_set_text_fmt(view.labels[i],
                          LV_SYMBOL_PLAY "  REC-20261012-10150%d-000.wav", i);
  lv_label_set_text(view.status, "Playing 00:03 / 00:12");
  lv_obj_add_state(view.older, LV_STATE_DISABLED);
  lv_obj_update_layout(view.root);
  for (int i = 0; i < 5; ++i) {
    assert(lv_obj_get_height(view.rows[i]) >= 46);
    lv_area_t a;
    lv_obj_get_coords(view.rows[i], &a);
    assert(a.x1 >= 0 && a.x2 < 410 && a.y1 >= 65 && a.y2 < 329);
    lv_obj_send_event(view.rows[i], LV_EVENT_CLICKED, nullptr);
  }
  assert(clicks == 5);
  lv_area_t a;
  lv_obj_get_coords(view.stop, &a);
  assert(a.y2 < 502);
  lv_obj_set_height(view.root,454);lv_obj_update_layout(view.root);
  lv_obj_get_coords(view.stop,&a);assert(a.y2<454);
  lv_refr_now(d);
  auto *f = std::fopen(argc > 1 ? argv[1] : "recorder-preview.ppm", "wb");
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
  lv_obj_delete(view.root);
  for (int i = 0; i < 30; ++i) {
    view.create(lv_screen_active(), nullptr, nullptr);
    lv_obj_delete(view.root);
  }
  puts("Recorder library: five touch targets, screen bounds, event dispatch "
       "and 30 open-close cycles passed");
  return 0;
}
