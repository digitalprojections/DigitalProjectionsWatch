#pragma once
#include "GameView.h"
#include "esp_brookesia.hpp"
namespace esp_brookesia::apps {
class Watch2048 final : public systems::phone::App {
    dpwatch::GameView view;
    lv_timer_t *timer = nullptr;
    static void poll(lv_timer_t *timer);

  public:
    Watch2048();
    bool run() override;
    bool back() override;
    bool pause() override;
    bool resume() override;
    bool close() override;
};
} // namespace esp_brookesia::apps
