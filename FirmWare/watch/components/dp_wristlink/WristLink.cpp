#include "WristLink.h"
#include "MotionService.h"
#include "WristLinkIcon.h"
#include "esp_lib_utils.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
namespace esp_brookesia::apps {
WristLink::WristLink()
    : App("WristLink", &wristlink::icon, true, false, true) {}
bool WristLink::startRadio() {
  if(radio)return true;
  wifi_mode_t mode;
  error = esp_wifi_get_mode(&mode);
  if (error == ESP_ERR_WIFI_NOT_INIT) {
    if (!esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"))
      esp_netif_create_default_wifi_sta();
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    error = esp_wifi_init(&config);
    if (error != ESP_OK)
      return false;
    error = esp_wifi_set_mode(WIFI_MODE_STA);
    if (error != ESP_OK)
      return false;
  }
  error = esp_wifi_start();
  if (error != ESP_OK && error != ESP_ERR_WIFI_CONN)
    return false;
  error = esp_now_init();
  if (error != ESP_OK)
    return false;
  esp_now_peer_info_t peer{};
  memset(peer.peer_addr, 255, 6);
  peer.ifidx = WIFI_IF_STA;
  peer.channel = 0;
  error = esp_now_add_peer(&peer);
  if (error != ESP_OK) {
    esp_now_deinit();
    return false;
  }
  error = esp_now_register_recv_cb(receive);
  if (error != ESP_OK) {
    esp_now_deinit();
    return false;
  }
  radio = true;
  return true;
}
void WristLink::stopRadio() {
  if (radio) {
    esp_now_unregister_recv_cb();
    esp_now_deinit();
  }
  radio = false;
  ackAt = 0;
}
void WristLink::receive(const esp_now_recv_info_t *, const uint8_t *b,
                        int size) {
  auto *self = instance;
  if (!self || size != sizeof(wristlink::Packet))
    return;
  wristlink::Packet p;
  memcpy(&p, b, sizeof(p));
  if (wristlink::valid(p) && p.type == 2 && p.session == self->packet.session) {
    self->ackAt = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    ++self->acknowledgements;
  }
}
bool WristLink::run() {
  instance = this;
  packet = {};
  packet.session = esp_random();
  if (!packet.session)
    packet.session = 1;
  paused = false;
  buttons.reset();
  sent = 0;
  acknowledgements = 0;
  view.open(lv_screen_active(), [](void*data,unsigned value){static_cast<WristLink*>(data)->packet.throttle=value;}, [](void*){dpwatch::requestMotionCalibration();}, this);
  startRadio();
  timer = lv_timer_create(poll, 40, this);
  return timer != nullptr;
}
void WristLink::poll(lv_timer_t *t) {
  static_cast<WristLink *>(lv_timer_get_user_data(t))->update();
}
void WristLink::update() {
  const auto m = dpwatch::motionSnapshot();
  auto now = static_cast<uint32_t>(esp_timer_get_time() / 1000);
  const auto action=buttons.update(m.bootPressed,now);
  if(action==wristlink::ButtonAction::Center)++packet.center;
  if(action==wristlink::ButtonAction::Pause)paused=!paused;
  packet.flags = (m.state == dpwatch::CalibrationState::Ready && m.ageMs < 200)
                     ? wristlink::Valid
                     : 0;
  if (paused)
    packet.flags |= wristlink::Paused;
  packet.ax = wristlink::quantize(m.sample.ax, 1000);
  packet.ay = wristlink::quantize(m.sample.ay, 1000);
  packet.az = wristlink::quantize(m.sample.az, 1000);
  packet.gx = wristlink::quantize(m.sample.gx, 100);
  packet.gy = wristlink::quantize(m.sample.gy, 100);
  packet.gz = wristlink::quantize(m.sample.gz, 100);
  ++packet.sequence;
  wristlink::seal(packet);
  if (radio) {
    uint8_t broadcast[6];
    memset(broadcast, 255, 6);
    error = esp_now_send(broadcast, reinterpret_cast<uint8_t *>(&packet),
                         sizeof(packet));
    if (error == ESP_OK)
      ++sent;
  }
  if (now - uiAt < 200)
    return;
  uiAt = now;
  bool linked = acknowledgements.load() && uint32_t(now - ackAt.load()) < 600;
  const char *state = !radio ? "Radio unavailable"
                      : m.state == dpwatch::CalibrationState::Loading
                          ? "Calibrating. Keep still..."
                      : !(packet.flags & wristlink::Valid)
                          ? "Motion needs calibration"
                      : !linked ? "Searching for console..."
                      : paused  ? "Connected - paused"
                                : "Connected - controlling";

  char line[180];
  snprintf(line, sizeof(line),
           "Accel g: %+.2f  %+.2f  %+.2f\nGyro deg/s: %+.1f  %+.1f  %+.1f",
           m.sample.ax, m.sample.ay, m.sample.az, m.sample.gx, m.sample.gy,
           m.sample.gz);
  view.update(state,line,packet.throttle,m.state!=dpwatch::CalibrationState::Loading && m.state!=dpwatch::CalibrationState::Unavailable);
}
bool WristLink::back() { return notifyCoreClosed(); }
bool WristLink::pause() {
  if (timer)
    lv_timer_pause(timer);
  stopRadio();
  return true;
}
bool WristLink::resume() {
  buttons.reset();
  startRadio();
  if (timer)
    lv_timer_resume(timer);
  return true;
}
bool WristLink::close() {
  if (timer) {
    lv_timer_delete(timer);
    timer = nullptr;
  }
  stopRadio();
  view.close();
  return true;
}
void WristLink::diagnostic() {
  auto *self = instance;
  uint8_t channel = 0;
  wifi_second_chan_t second;
  esp_wifi_get_channel(&channel, &second);
  printf("WRISTLINK active=%d radio=%d linked=%d channel=%u sent=%u ack=%u "
         "flags=%u throttle=%u center=%u error=%s\n",
         self && self->timer, self && self->radio,
         self && self->acknowledgements.load() &&
             uint32_t(esp_timer_get_time() / 1000 - self->ackAt.load()) < 600,
         channel, self ? self->sent : 0,
         self ? self->acknowledgements.load() : 0,
         self ? self->packet.flags : 0, self ? self->packet.throttle : 0,
         self ? self->packet.center : 0,
         self ? esp_err_to_name(self->error) : "ESP_OK");
}
ESP_UTILS_REGISTER_PLUGIN_WITH_CONSTRUCTOR(systems::base::App, WristLink,
                                           "WristLink", []() {
                                             static auto app =
                                                 std::make_shared<WristLink>();
                                             return app;
                                           })
} // namespace esp_brookesia::apps
