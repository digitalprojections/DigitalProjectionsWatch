#include "watch_console.hpp"
#include "MotionService.h"
#include "PhoneSetup.h"
#include "Settings.hpp"
#include "Recorder.hpp"
#include "TimeService.h"
#include "WatchStatusGeometry.h"
#include "storage_service.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_lv_adapter.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
namespace {
using namespace esp_brookesia::systems;
phone::Phone *systemPhone = nullptr;
struct AppNavigation { int id; bool home; bool start; };
void applyNavigation(void *data) {
    const AppNavigation action=*static_cast<AppNavigation *>(data);
    delete static_cast<AppNavigation *>(data);
    bool ok;
    if(action.home) ok=systemPhone->sendNavigateEvent(base::Manager::NavigateType::HOME);
    else {
        const base::Context::AppEventData event{action.id,
            action.start?base::Context::AppEventType::START:base::Context::AppEventType::STOP,nullptr};
        ok=systemPhone->sendAppEvent(&event);
    }
    std::printf("WATCH navigation=%s\n",ok?"ok":"error");
}
bool queueNavigation(int id,bool home,bool start) {
    // App transitions run on the normal UI task, not the small console stack.
    auto *action=new(std::nothrow) AppNavigation{id,home,start};
    if(!action)return false;
    if(lv_async_call(applyNavigation,action)!=LV_RESULT_OK){delete action;return false;}
    return true;
}

int clockCommand(int argc,char**argv){
 if(argc==2&&!strcmp(argv[1],"sync"))dpwatch::requestTimeSync();
 else if(argc==2&&!strcmp(argv[1],"page")){
  if(esp_lv_adapter_lock(1000)!=ESP_OK)return 1;
  lv_async_call([](void*){esp_brookesia::apps::Settings::requestInstance()->showTimePage();},nullptr);
  esp_lv_adapter_unlock();
 }else if(argc==3&&!strcmp(argv[1],"zone")){
  char*end=nullptr;const long z=strtol(argv[2],&end,10);if(*end||z<0||z>=dpwatch::timeZoneCount)return 1;
  if(esp_lv_adapter_lock(1000)!=ESP_OK)return 1;
  lv_async_call([](void*data){esp_brookesia::apps::TimePage::instance().selectZone(static_cast<unsigned>(reinterpret_cast<uintptr_t>(data)-1));},reinterpret_cast<void*>(static_cast<uintptr_t>(z+1)));
  esp_lv_adapter_unlock();
 }else if(argc==2&&!strcmp(argv[1],"ui")){
  if(esp_lv_adapter_lock(1000)!=ESP_OK)return 1;
  auto*bar=systemPhone->getDisplay().getStatusBar();lv_area_t c{},b{},w{};
  const bool found=bar&&bar->getWidgetBounds(c,b,w);
  const bool safe=found&&dpwatch::statusBoundsSafe(c.x1,c.y1,c.x2,c.y2)&&dpwatch::statusBoundsSafe(b.x1,b.y1,b.x2,b.y2)&&dpwatch::statusBoundsSafe(w.x1,w.y1,w.x2,w.y2);
  printf("CLOCK_UI visible=%d safe=%d clock=%d,%d,%d,%d battery=%d,%d,%d,%d wifi=%d,%d,%d,%d\n",bar&&bar->checkVisible(),safe,static_cast<int>(c.x1),static_cast<int>(c.y1),static_cast<int>(c.x2),static_cast<int>(c.y2),static_cast<int>(b.x1),static_cast<int>(b.y1),static_cast<int>(b.x2),static_cast<int>(b.y2),static_cast<int>(w.x1),static_cast<int>(w.y1),static_cast<int>(w.x2),static_cast<int>(w.y2));
  esp_lv_adapter_unlock();
 }else if(argc!=1)return 1;
 const auto s=dpwatch::timeSnapshot();time_t t=static_cast<time_t>(s.epoch);tm date{};localtime_r(&t,&date);char local[32];strftime(local,sizeof(local),"%Y-%m-%dT%H:%M:%S%z",&date);
 printf("CLOCK state=%u source=%u valid=%d online=%d zone=%u epoch=%llu local=%s rtc_available=%d rtc_saved=%d rtc_epoch=%llu last_sync=%llu error=%s rtc_error=%s\n",static_cast<unsigned>(s.state),static_cast<unsigned>(s.source),s.valid,s.online,s.zone,static_cast<unsigned long long>(s.epoch),s.valid?local:"unavailable",s.rtcAvailable,s.rtcSaved,static_cast<unsigned long long>(s.rtcEpoch),static_cast<unsigned long long>(s.lastSync),esp_err_to_name(s.error),esp_err_to_name(s.rtcError));
 return 0;
}
int recorder(int argc, char **argv) {
    if(argc>2 || esp_lv_adapter_lock(1000)!=ESP_OK)return 1;
    auto *app=esp_brookesia::apps::Recorder::requestInstance();
    const bool ok=systemPhone->getManager().getActiveApp()==app && app->diagnostic(argc==2?argv[1]:"status");
    esp_lv_adapter_unlock();return ok?0:1;
}
int motion(int, char **) {
    const auto s = dpwatch::motionSnapshot();
    constexpr const char *states[] = {"needed", "loading", "ready", "unavailable", "error"};
    std::printf("MOTION state=%s generation=%u age_ms=%u accel_g=%.5f,%.5f,%.5f gyro_dps=%.5f,%.5f,%.5f boot=%d\n",
        states[static_cast<unsigned>(s.state)], s.generation, s.ageMs,
        s.sample.ax,s.sample.ay,s.sample.az,s.sample.gx,s.sample.gy,s.sample.gz,s.bootPressed);
    return 0;
}
int phoneSetup(int argc, char **argv) {
 if(argc==2 && std::strcmp(argv[1],"start")==0) dpwatch::startPhoneSetup();
 else if(argc==2 && std::strcmp(argv[1],"stop")==0) dpwatch::stopPhoneSetup();
 else if(argc==2 && std::strcmp(argv[1],"page")==0) {
  if(esp_lv_adapter_lock(1000)!=ESP_OK)return 1;
  esp_brookesia::apps::Settings::requestInstance()->showBluetoothPage();
  esp_lv_adapter_unlock();
 } else if(argc!=1)return 1;
 const auto s=dpwatch::phoneSetupSnapshot();
 std::printf("PHONE_SETUP state=%u can_start=%d can_stop=%d free_internal=%u largest_internal=%u\n",static_cast<unsigned>(s.state),s.canStart,s.canStop,static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
 return 0;
}
int calibrate(int, char **) { dpwatch::requestMotionCalibration(); std::puts("Calibration requested; keep flat and still."); return 0; }
int storage(int argc, char **argv) {
    if (argc == 2 && std::strcmp(argv[1], "test") == 0) {
        storage_service_benchmark_t result{};
        const auto error = storage_service_run_benchmark(16384, &result);
        std::printf("SD_TEST error=%s written=%u read=%u crc_match=%d\n", esp_err_to_name(error),
            static_cast<unsigned>(result.written_bytes), static_cast<unsigned>(result.read_bytes),
            result.expected_crc32==result.actual_crc32);
        return error == ESP_OK ? 0 : 1;
    }
    storage_service_mount();
    storage_service_info_t info{};
    const auto error=storage_service_get_info(&info);
    std::printf("SD state=%s error=%s capacity=%llu free=%llu\n", storage_service_state_name(info.state),
        esp_err_to_name(error==ESP_OK?info.last_error:error),
        static_cast<unsigned long long>(info.capacity_bytes), static_cast<unsigned long long>(info.free_bytes));
    return error==ESP_OK && info.state==STORAGE_SERVICE_STATE_MOUNTED ? 0 : 1;
}
int apps(int argc, char **argv) {
    if (esp_lv_adapter_lock(1000) != ESP_OK) return 1;
    bool result = true;
    if (argc == 1) {
        for (int id=1;id<=64;++id) {
            auto *app = systemPhone->getManager().getInstalledApp(id);
            if (app) std::printf("APP %d %s\n", id, app->getName());
        }
    } else if (argc == 2 && std::strcmp(argv[1], "home") == 0) {
        result = queueNavigation(0,true,false);
    } else if (argc == 3 && (std::strcmp(argv[1], "open") == 0 || std::strcmp(argv[1], "close") == 0)) {
        char *end = nullptr;
        const long id = std::strtol(argv[2], &end, 10);
        if (*end || id<1 || id>64 || !systemPhone->getManager().getInstalledApp(static_cast<int>(id))) result=false;
        else {
            result=queueNavigation(static_cast<int>(id),false,std::strcmp(argv[1],"open")==0);
        }
    } else result = false;
    const auto *active = systemPhone->getManager().getActiveApp();
    const int activeId = active ? active->getId() : 0;
    esp_lv_adapter_unlock();
    std::printf("WATCH %s active_id=%d free_internal=%u\n", result?"ok":"error", activeId, static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
    return result ? 0 : 1;
}
}
void startWatchConsole(esp_brookesia::systems::phone::Phone *phone) {
    systemPhone = phone;
    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    config.prompt = "dp-watch>";
    esp_console_dev_usb_serial_jtag_config_t hardware = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    if (esp_console_new_repl_usb_serial_jtag(&hardware, &config, &repl) != ESP_OK) return;
    const esp_console_cmd_t commands[] = {
        {.command="clock", .help="Time status/sync/page/zone INDEX/ui", .hint=nullptr, .func=clockCommand, .argtable=nullptr},
        {.command="recorder", .help="Recorder UI status/library/play1..5/stop/back/older/record", .hint=nullptr, .func=recorder, .argtable=nullptr},
        {.command="phone_setup", .help="Bluetooth Wi-Fi setup status/start/stop/page", .hint=nullptr, .func=phoneSetup, .argtable=nullptr},
        {.command="sd", .help="Show SD status; sd test checks a private temporary file", .hint=nullptr, .func=storage, .argtable=nullptr},
        {.command="motion", .help="Show shared motion status and calibration generation", .hint=nullptr, .func=motion, .argtable=nullptr},
        {.command="calibrate", .help="Calibrate the shared motion sensor while flat and still", .hint=nullptr, .func=calibrate, .argtable=nullptr},
        {.command="watch", .help="List apps, or watch open ID / close ID / home", .hint=nullptr, .func=apps, .argtable=nullptr}
    };
    for (const auto &command : commands) ESP_ERROR_CHECK(esp_console_cmd_register(&command));
    ESP_ERROR_CHECK(esp_console_register_help_command());
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
