#include "TimeView.h"
#include "SettingsUI.hpp"
#include "WatchStatusGeometry.h"
#include <array>
#include <cassert>
#include <cstdio>
static std::array<uint16_t, 410 * 502> pixels{};
static std::array<uint16_t, 410 * 40> buffer{};
static int starts = 0; static unsigned selectedZone=999;
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
  auto*bar=lv_obj_create(lv_screen_active());lv_obj_remove_style_all(bar);lv_obj_set_size(bar,410,dpwatch::statusBarHeight);lv_obj_set_style_bg_color(bar,lv_color_hex(0x1a1a1a),0);lv_obj_set_style_bg_opa(bar,LV_OPA_COVER,0);lv_obj_set_style_text_color(bar,lv_color_white(),0);
  auto*c=lv_label_create(bar);lv_label_set_text(c,"10:42");lv_obj_set_style_text_font(c,&lv_font_montserrat_20,0);lv_obj_align(c,LV_ALIGN_CENTER,0,0);
  auto*w=lv_label_create(bar);lv_label_set_text(w,LV_SYMBOL_WIFI);lv_obj_align(w,LV_ALIGN_LEFT_MID,40,0);
  auto*b=lv_label_create(bar);lv_label_set_text(b,"85% " LV_SYMBOL_BATTERY_FULL);lv_obj_align(b,LV_ALIGN_RIGHT_MID,-40,0);
  auto*app=lv_obj_create(lv_screen_active());lv_obj_remove_style_all(app);lv_obj_set_size(app,410,454);lv_obj_set_pos(app,0,48);
  auto*page=esp_brookesia::apps::settings_ui::create_page(app);esp_brookesia::apps::settings_ui::create_header(page,"Date & time");
  auto*body=lv_obj_create(page);lv_obj_remove_style_all(body);lv_obj_set_size(body,410,382);lv_obj_align(body,LV_ALIGN_BOTTOM_MID,0,0);
  dpwatch::TimeView view;view.open(body,[](void*){++starts;},[](void*,unsigned z){selectedZone=z;},nullptr);
  auto*root=lv_obj_get_child(body,0);auto*sync=lv_obj_get_child(root,6);
  lv_obj_send_event(sync,LV_EVENT_CLICKED,nullptr);assert(starts==0&&lv_obj_has_state(sync,LV_STATE_DISABLED));
  dpwatch::TimeViewData data;data.online=true;data.state=dpwatch::TimeState::Loading;view.update(data);lv_obj_send_event(sync,LV_EVENT_CLICKED,nullptr);assert(starts==0);
  data.state=dpwatch::TimeState::Success;data.valid=true;data.clock="10:42:08";data.date="Mon, 12 Oct 2026";data.detail="Source: internet\nRTC: saved";view.update(data);
  lv_obj_send_event(sync,LV_EVENT_CLICKED,nullptr);assert(starts==1&&lv_obj_has_state(sync,LV_STATE_DISABLED));
  view.selectZone(2);assert(selectedZone==2);view.selectZone(dpwatch::timeZoneCount);assert(selectedZone==2);
  view.update(data);lv_obj_update_layout(page);lv_obj_update_layout(bar);
  lv_area_t a;lv_obj_get_coords(sync,&a);assert(a.y2<478&&lv_obj_get_height(sync)>=54);
  for(auto*item:{c,w,b}){lv_obj_get_coords(item,&a);assert(dpwatch::statusBoundsSafe(a.x1,a.y1,a.x2,a.y2));}
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
  view.close();view.close();
  for(int i=0;i<30;++i){view.open(body,nullptr,nullptr,nullptr);view.close();assert(lv_obj_get_child_count(body)==0);}
  puts("Time view: offline/loading/success action gates, timezone selection, top safe bounds and 30 open-close cycles passed");
  return 0;
}
