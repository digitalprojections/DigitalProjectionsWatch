#pragma once
#include "lvgl.h"
namespace dpwatch {
struct RecorderLibraryView {
  lv_obj_t *root = nullptr, *back = nullptr, *status = nullptr,
           *older = nullptr, *stop = nullptr;
  lv_obj_t *rows[5] = {}, *labels[5] = {};
  void create(lv_obj_t *screen, lv_event_cb_t callback, void *user) {
    root = lv_obj_create(screen);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(root, lv_color_white(), 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    auto button = [&](const char *text, int width, int height, int x, int y) {
      lv_obj_t *b = lv_button_create(root);
      lv_obj_set_size(b, width, height);
      lv_obj_align(b, LV_ALIGN_TOP_MID, x, y);
      lv_obj_t *l = lv_label_create(b);
      lv_label_set_text(l, text);
      lv_obj_center(l);
      lv_obj_add_event_cb(b, callback, LV_EVENT_CLICKED, user);
      return b;
    };
    back = button(LV_SYMBOL_LEFT, 56, 48, -146, 42);
    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Recordings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 25, 53);
    for (size_t i = 0; i < 5; ++i) {
      rows[i] = button("", 350, 46, 0, 100 + static_cast<int>(i) * 51);
      labels[i] = lv_obj_get_child(rows[i], 0);
      lv_obj_set_width(labels[i], 326);
      lv_label_set_long_mode(labels[i], LV_LABEL_LONG_DOT);
      lv_obj_set_style_text_font(labels[i], &lv_font_montserrat_16, 0);
    }
    status = lv_label_create(root);
    lv_obj_set_size(status, 350, 48);
    lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_18, 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 359);
    stop = button("Stop", 162, 52, -92, 419);
    older = button("Older", 162, 52, 92, 419);
    lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
  }
};
} // namespace dpwatch
