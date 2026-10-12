#pragma once
#include "lvgl.h"
namespace wristlink {
class ControllerView {
 lv_obj_t *root=nullptr,*status=nullptr,*sensors=nullptr,*power=nullptr,*slider=nullptr,*calibrate=nullptr;
 void* user=nullptr;void(*throttle)(void*,unsigned)=nullptr;void(*calibration)(void*)=nullptr;
public:
 lv_obj_t* object()const{return root;}
 void open(lv_obj_t*parent,void(*throttleCb)(void*,unsigned),void(*calibrateCb)(void*),void*data){
  user=data;throttle=throttleCb;calibration=calibrateCb;
  root=lv_obj_create(parent);lv_obj_remove_style_all(root);lv_obj_set_size(root,LV_PCT(100),LV_PCT(100));lv_obj_set_style_bg_color(root,lv_color_black(),0);lv_obj_set_style_bg_opa(root,LV_OPA_COVER,0);lv_obj_set_style_text_color(root,lv_color_white(),0);lv_obj_set_style_text_font(root,&lv_font_montserrat_18,0);lv_obj_set_style_pad_all(root,20,0);lv_obj_set_style_pad_bottom(root,32,0);lv_obj_set_style_pad_row(root,10,0);lv_obj_set_flex_flow(root,LV_FLEX_FLOW_COLUMN);lv_obj_set_flex_align(root,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE);
  auto label=[&](const char*t){auto*l=lv_label_create(root);lv_label_set_text(l,t);lv_obj_set_width(l,340);lv_obj_set_style_text_align(l,LV_TEXT_ALIGN_CENTER,0);return l;};
  auto*title=label("WristLink");lv_obj_set_style_text_font(title,&lv_font_montserrat_32,0);status=label("Searching for console...");sensors=label("Accel g: -- -- --\nGyro deg/s: -- -- --");lv_obj_set_style_text_font(sensors,&lv_font_montserrat_16,0);power=label("Throttle 50%");slider=lv_slider_create(root);lv_obj_set_size(slider,320,28);lv_slider_set_range(slider,0,100);lv_slider_set_value(slider,50,LV_ANIM_OFF);
  lv_obj_add_event_cb(slider,[](lv_event_t*e){auto*self=static_cast<ControllerView*>(lv_event_get_user_data(e));if(self->throttle)self->throttle(self->user,lv_slider_get_value(self->slider)*10);},LV_EVENT_VALUE_CHANGED,this);
  label("BOOT: center steering\nHold BOOT: pause / resume");calibrate=lv_button_create(root);lv_obj_set_size(calibrate,310,40);auto*text=lv_label_create(calibrate);lv_label_set_text(text,"Calibrate motion");lv_obj_center(text);
  lv_obj_add_event_cb(calibrate,[](lv_event_t*e){auto*self=static_cast<ControllerView*>(lv_event_get_user_data(e));if(!lv_obj_has_state(self->calibrate,LV_STATE_DISABLED)&&self->calibration)self->calibration(self->user);},LV_EVENT_CLICKED,this);
 }
 void update(const char*state,const char*motion,unsigned value,bool canCalibrate){if(!root)return;lv_label_set_text(status,state);lv_label_set_text(sensors,motion);char line[32];lv_snprintf(line,sizeof(line),"Throttle %u%%",value/10);lv_label_set_text(power,line);if(canCalibrate)lv_obj_remove_state(calibrate,LV_STATE_DISABLED);else lv_obj_add_state(calibrate,LV_STATE_DISABLED);}
 void close(){if(root)lv_obj_delete(root);root=nullptr;}
};
}
