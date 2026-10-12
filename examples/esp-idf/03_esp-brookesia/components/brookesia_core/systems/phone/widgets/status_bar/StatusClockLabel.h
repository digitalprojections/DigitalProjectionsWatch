#pragma once
#include "lvgl.h"
namespace esp_brookesia::systems::phone {
inline lv_obj_t* createStatusClockLabel(lv_obj_t*parent,const lv_font_t*font){
 auto*label=lv_label_create(parent);lv_obj_remove_style_all(label);
 lv_obj_set_style_text_font(label,font,0);lv_obj_set_size(label,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
 lv_obj_add_flag(label,LV_OBJ_FLAG_IGNORE_LAYOUT);lv_obj_remove_flag(label,LV_OBJ_FLAG_SCROLLABLE);
 lv_label_set_text(label,"--:--");lv_obj_align(label,LV_ALIGN_CENTER,0,0);return label;
}
}
