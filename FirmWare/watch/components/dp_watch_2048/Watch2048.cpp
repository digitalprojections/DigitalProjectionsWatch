#include "Watch2048.h"
#include "GameIcon.h"
#include "MotionService.h"
#include "esp_lib_utils.h"
#include "esp_random.h"
namespace esp_brookesia::apps {
Watch2048::Watch2048() : App("2048", &dpwatch::gameIcon, true, false, true) {}
bool Watch2048::run() {
    view.open(lv_screen_active(), esp_random());
    timer = lv_timer_create(poll, 20, this);
    return timer != nullptr;
}
void Watch2048::poll(lv_timer_t *timer) {
    auto *app = static_cast<Watch2048 *>(lv_timer_get_user_data(timer));
    const auto data = dpwatch::motionSnapshot();
    const auto now = lv_tick_get();
    app->view.button(data.bootPressed, now);
}
bool Watch2048::back() { return notifyCoreClosed(); }
bool Watch2048::pause() {
    if (timer)
        lv_timer_pause(timer);
    return true;
}
bool Watch2048::resume() {
    view.resumeInput(lv_tick_get());
    if (timer)
        lv_timer_resume(timer);
    return true;
}
bool Watch2048::close() {
    if (timer) {
        lv_timer_delete(timer);
        timer = nullptr;
    }
    view.close();
    return true;
}
ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, Watch2048, "2048",
                                           []() { static auto instance = std::make_shared<Watch2048>(); return instance; })
} // namespace esp_brookesia::apps
