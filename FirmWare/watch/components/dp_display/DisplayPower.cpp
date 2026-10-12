#include "DisplayPower.h"
#include "DisplayPowerRules.h"
#include "bsp/display.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include <atomic>
#include <cstdio>
namespace dpwatch {
namespace {
DisplayPowerRules state;
PowerTap powerTap;
lv_obj_t* shield=nullptr;
lv_timer_t* timer=nullptr;
std::atomic<bool> consumeBoot{false};
esp_err_t lastError=ESP_OK;
bool down(){return gpio_get_level(GPIO_NUM_0)==0;}
void applyShield(){consumeBoot.store(state.consumesBoot());if(shield){if(state.consumesBoot())lv_obj_remove_flag(shield,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(shield,LV_OBJ_FLAG_HIDDEN);}}
void poll(lv_timer_t*){
 if(powerTap.update(gpio_get_level(GPIO_NUM_10)!=0,lv_tick_get())){if(state.off())wakeDisplay();else turnDisplayOff();}
 if(state.poll(down())){wakeDisplay();}
 applyShield();
}
}
bool startDisplayPower(){
 if(timer)return true;
 gpio_config_t power{};power.pin_bit_mask=1ULL<<GPIO_NUM_10;power.mode=GPIO_MODE_INPUT;
 if(gpio_config(&power)!=ESP_OK)return false;
 powerTap.reset(gpio_get_level(GPIO_NUM_10)!=0);
 shield=lv_obj_create(lv_layer_sys());
 lv_obj_remove_style_all(shield);lv_obj_set_size(shield,LV_PCT(100),LV_PCT(100));lv_obj_set_pos(shield,0,0);
 lv_obj_set_style_bg_color(shield,lv_color_black(),0);lv_obj_set_style_bg_opa(shield,LV_OPA_COVER,0);
 lv_obj_remove_flag(shield,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(shield,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_flag(shield,LV_OBJ_FLAG_HIDDEN);
 lv_obj_add_event_cb(shield,[](lv_event_t*){if(state.off())wakeDisplay();},LV_EVENT_RELEASED,nullptr);
 timer=lv_timer_create(poll,20,nullptr);return timer!=nullptr;
}
esp_err_t turnDisplayOff(){
 if(!timer)return ESP_ERR_INVALID_STATE;
 if(state.off())return ESP_OK;
 lastError=bsp_display_power_set(false);
 if(lastError==ESP_OK){state.slept(down());applyShield();}
 return lastError;
}
esp_err_t wakeDisplay(){
 if(!timer)return ESP_ERR_INVALID_STATE;
 if(!state.off())return ESP_OK;
 lastError=bsp_display_power_set(true);
 if(lastError==ESP_OK){state.woke(down());applyShield();lv_obj_invalidate(lv_screen_active());}
 return lastError;
}
bool displayConsumesBoot(){return consumeBoot.load();}
void displayPowerDiagnostic(){printf("DISPLAY state=%s brightness=%d consumes_boot=%d pwr=%d error=%s\n",state.off()?"off":"on",bsp_display_brightness_get(),consumeBoot.load(),gpio_get_level(GPIO_NUM_10),esp_err_to_name(lastError));}
}
