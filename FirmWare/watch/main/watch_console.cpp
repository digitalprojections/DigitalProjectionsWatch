#include "watch_console.hpp"
#include "MotionService.h"
#include "PhoneSetup.h"
#include "Settings.hpp"
#include "storage_service.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_lv_adapter.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
using namespace esp_brookesia::systems;
phone::Phone *systemPhone = nullptr;
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
        result = systemPhone->sendNavigateEvent(base::Manager::NavigateType::HOME);
    } else if (argc == 3 && (std::strcmp(argv[1], "open") == 0 || std::strcmp(argv[1], "close") == 0)) {
        char *end = nullptr;
        const long id = std::strtol(argv[2], &end, 10);
        if (*end || id<1 || id>64 || !systemPhone->getManager().getInstalledApp(static_cast<int>(id))) result=false;
        else {
            const base::Context::AppEventData event{static_cast<int>(id),
                std::strcmp(argv[1],"open")==0 ? base::Context::AppEventType::START : base::Context::AppEventType::STOP, nullptr};
            result=systemPhone->sendAppEvent(&event);
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
