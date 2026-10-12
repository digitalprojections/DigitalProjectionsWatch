#include "GameView.h"
#include <array>
#include <cstdio>
#include <cassert>
static std::array<uint16_t,410*502> pixels{};
static std::array<uint16_t,410*40> buffer{};
static void flush(lv_display_t*d,const lv_area_t*a,uint8_t*raw){auto *source=reinterpret_cast<uint16_t*>(raw);for(int y=a->y1;y<=a->y2;++y)for(int x=a->x1;x<=a->x2;++x)pixels[y*410+x]=*source++;lv_display_flush_ready(d);}
static lv_point_t point{};
static bool pressed = false;
static void readTouch(lv_indev_t*, lv_indev_data_t *data) {
 data->point = point;
 data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static void touch(lv_indev_t *input, int x, int y, bool down) {
 point = {x,y}; pressed = down; lv_tick_inc(20); lv_indev_read(input);
}
static void swipe(lv_indev_t *input, int x, int y, int endX, int endY) {
 touch(input,x,y,true); touch(input,endX,endY,true); touch(input,endX,endY,false);
}
int main(int argc,char**argv){
 lv_init();auto*d=lv_display_create(410,502);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(d,flush);lv_display_set_buffers(d,buffer.data(),nullptr,buffer.size()*2,LV_DISPLAY_RENDER_MODE_PARTIAL);
 auto *input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(input,readTouch);
 dpwatch::GameView view;view.open(lv_screen_active(),42);lv_obj_update_layout(lv_screen_active());
 // Exercise LVGL hit testing over the actual tile widgets, not just the classifier.
 const int ends[4][2]={{100,220},{300,220},{200,120},{200,320}};
 for (const auto &end:ends) {
  view.game.cells={};view.game.cells[5]=2;view.game.cells[6]=2;
  const auto saved=view.game.cells;
  swipe(input,200,220,end[0],end[1]);assert(view.game.cells!=saved);
 }
 auto saved=view.game.cells;swipe(input,200,220,210,225);assert(view.game.cells==saved);
 swipe(input,200,220,250,270);assert(view.game.cells==saved);
 view.button(false,0);view.button(true,100);view.button(true,140);
 view.button(false,200);view.button(false,240);view.button(false,610);
 swipe(input,200,220,100,220);assert(view.game.cells==saved);
 view.button(true,1100);view.button(true,1140);view.button(false,1200);view.button(false,1240);view.button(false,1610);
 view.game.cells={};view.game.cells[5]=2;view.game.cells[6]=2;saved=view.game.cells;
 swipe(input,200,220,100,220);assert(view.game.cells!=saved);
 // App resume discards an incomplete contact.
 saved=view.game.cells;touch(input,200,220,true);view.resumeInput(2000);touch(input,100,220,false);assert(view.game.cells==saved);
 view.button(false,3200);view.button(true,3300);view.button(true,3340);view.button(true,5400);
 unsigned occupied=0;for(auto cell:view.game.cells)occupied+=cell!=0;
 assert(occupied==2 && view.game.score==0);
 view.close();view.close();assert(lv_obj_get_child_count(lv_screen_active())==0);
 for(int i=0;i<30;++i){view.open(lv_screen_active(),42);view.close();assert(lv_obj_get_child_count(lv_screen_active())==0);}
 std::puts("LVGL: real pointer swipes in four directions, taps, diagonals, pause/resume, cancelled contact, new game and 30 reopen cycles passed.");
 view.open(lv_screen_active(),42);view.game.cells={2,4,8,16,32,64,128,256,512,1024,2048,0,2,4,0,0};
 view.close();view.open(lv_screen_active(),42);
 lv_refr_now(d);auto*f=std::fopen(argc>1?argv[1]:"preview.ppm","wb");if(!f)return 1;std::fprintf(f,"P6\n410 502\n255\n");
 for(auto c:pixels){unsigned char rgb[]={static_cast<unsigned char>(((c>>11)&31)*255/31),static_cast<unsigned char>(((c>>5)&63)*255/63),static_cast<unsigned char>((c&31)*255/31)};std::fwrite(rgb,1,3,f);}std::fclose(f);return 0;
}
