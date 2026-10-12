#pragma once
#include "WristLinkCore.h"
#include "WristLinkView.h"
#include "esp_brookesia.hpp"
#include "esp_now.h"
#include "esp_wifi.h"
#include <atomic>
namespace esp_brookesia::apps {
class WristLink final : public systems::phone::App {
  inline static WristLink *instance = nullptr;
  wristlink::ControllerView view;
  lv_timer_t *timer = nullptr;
  wristlink::Packet packet{};
  std::atomic<uint32_t> ackAt{0};
  std::atomic<unsigned> acknowledgements{0};
  wristlink::Buttons buttons;
  bool radio = false, paused = false;
  wifi_ps_type_t savedPowerMode=WIFI_PS_NONE;
  bool powerManaged=false;
  uint32_t uiAt = 0;
  unsigned sent = 0;
  esp_err_t error = ESP_OK;
  static void poll(lv_timer_t *);
  static void receive(const esp_now_recv_info_t *, const uint8_t *, int);
  bool startRadio();
  void stopRadio();
  void update();

public:
  WristLink();
  bool run() override;
  bool back() override;
  bool pause() override;
  bool resume() override;
  bool close() override;
  static void diagnostic();
};
} // namespace esp_brookesia::apps
