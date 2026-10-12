#include "TimeService.h"
#include "bsp/esp-bsp.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "nvs.h"
#include <atomic>
#include <cstdlib>
#include <sys/time.h>
namespace dpwatch {
namespace {
constexpr const char *TAG = "DP:Time";
constexpr unsigned char rtcMarker = 0xd7;
const char *servers[] = {"pool.ntp.org", "time.google.com",
                         "time.cloudflare.com"};
i2c_master_dev_handle_t rtc = nullptr;
TimeModel model;
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
TimeSnapshot published;
std::atomic<unsigned> zone{0};
std::atomic<uint64_t> received{0};
std::atomic<bool> requested{false};
bool ready = false, ntpReady = false;
unsigned server = 0;
uint64_t savedRtc = 0;
esp_err_t syncError = ESP_OK, rtcError = ESP_OK;
unsigned millis() { return static_cast<unsigned>(esp_timer_get_time() / 1000); }
esp_err_t readReg(unsigned char reg, unsigned char *data, size_t n) {
  return rtc ? i2c_master_transmit_receive(rtc, &reg, 1, data, n, 100)
             : ESP_ERR_NOT_FOUND;
}
esp_err_t writeReg(unsigned char reg, unsigned char value) {
  unsigned char b[] = {reg, value};
  return rtc ? i2c_master_transmit(rtc, b, 2, 100) : ESP_ERR_NOT_FOUND;
}
esp_err_t readRtc(uint64_t &epoch) {
  unsigned char control = 0, marker = 0, raw[7];
  ESP_RETURN_ON_ERROR(readReg(0, &control, 1), TAG, "RTC control read failed");
  ESP_RETURN_ON_ERROR(readReg(3, &marker, 1), TAG, "RTC marker read failed");
  if (marker != rtcMarker || (control & 0xa2))
    return ESP_ERR_INVALID_STATE;
  ESP_RETURN_ON_ERROR(readReg(4, raw, 7), TAG, "RTC calendar read failed");
  RtcCalendar date;
  return decodeRtc(raw, date) && toEpoch(date, epoch)
             ? ESP_OK
             : ESP_ERR_INVALID_RESPONSE;
}
esp_err_t writeRtc(uint64_t epoch) {
  RtcCalendar date;
  unsigned char raw[8] = {4}, control = 0;
  if (!fromEpoch(epoch, date) || !encodeRtc(date, raw + 1))
    return ESP_ERR_INVALID_ARG;
  esp_err_t result = readReg(0, &control, 1);
  if (result != ESP_OK)
    return result;
  if (result == ESP_OK)
    result =
        writeReg(3, 0); // Invalidate until the complete UTC write is verified.
  const unsigned char running = control & ~0xa2U;
  if (result == ESP_OK)
    result = writeReg(0, running | 0x20);
  if (result == ESP_OK)
    result = i2c_master_transmit(rtc, raw, sizeof(raw), 100);
  const esp_err_t resume = writeReg(0, running);
  if (result == ESP_OK)
    result = resume;
  if (result == ESP_OK)
    result = writeReg(3, rtcMarker);
  uint64_t verify = 0;
  if (result == ESP_OK)
    result = readRtc(verify);
  if (result == ESP_OK && (verify < epoch || verify > epoch + 2))
    result = ESP_ERR_INVALID_RESPONSE;
  if (result == ESP_OK)
    savedRtc = verify;
  else
    (void)writeReg(3, 0);
  return result;
}
void notification(timeval *tv) {
  if (tv)
    received.store(static_cast<uint64_t>(tv->tv_sec));
}
void gotIp(void *, esp_event_base_t, int32_t, void *) { requested.store(true); }
esp_err_t configureNtp() {
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(servers[server]);
  config.start = false;
  config.wait_for_sync = false;
  config.sync_cb = notification;
  const auto result = esp_netif_sntp_init(&config);
  ntpReady = result == ESP_OK;
  return result;
}
void publish(bool online) {
  TimeSnapshot s;
  s.state = ready ? model.state : TimeState::Unavailable;
  s.source = model.source;
  s.valid = model.valid;
  s.online = online;
  s.rtcAvailable = rtc != nullptr;
  s.rtcSaved = savedRtc > 0;
  s.rtcEpoch = savedRtc;
  s.lastSync = model.lastSync;
  s.epoch = s.valid ? static_cast<uint64_t>(time(nullptr)) : 0;
  s.zone = zone.load();
  s.error = syncError;
  s.rtcError = rtcError;
  portENTER_CRITICAL(&lock);
  published = s;
  portEXIT_CRITICAL(&lock);
}
} // namespace
esp_err_t startTimeService() {
  unsigned char savedZone = 0;
  nvs_handle_t h;
  if (nvs_open("dp_time", NVS_READONLY, &h) == ESP_OK) {
    (void)nvs_get_u8(h, "zone", &savedZone);
    nvs_close(h);
  }
  if (savedZone >= timeZoneCount)
    savedZone = 0;
  zone.store(savedZone);
  setenv("TZ", timeZones[savedZone].rule, 1);
  tzset();
  auto bus = bsp_i2c_get_handle();
  if (bus && i2c_master_probe(bus, 0x51, 100) == ESP_OK) {
    i2c_device_config_t config{};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = 0x51;
    config.scl_speed_hz = 100000;
    rtcError = i2c_master_bus_add_device(bus, &config, &rtc);
    uint64_t epoch = 0;
    if (rtcError == ESP_OK)
      rtcError = readRtc(epoch);
    if (rtcError == ESP_OK) {
      timeval tv{};
      tv.tv_sec = static_cast<time_t>(epoch);
      settimeofday(&tv, nullptr);
      savedRtc = epoch;
      model.restored();
      ESP_LOGI(TAG, "Restored UTC time from RTC");
    }
  } else
    rtcError = ESP_ERR_NOT_FOUND;
  syncError = configureNtp();
  if (syncError == ESP_OK)
    syncError = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, gotIp,
                                           nullptr);
  ready = syncError == ESP_OK;
  publish(false);
  return syncError;
}
void pollTimeService(bool connected) {
  esp_netif_ip_info_t ip{};
  auto *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  connected = connected && netif &&
              esp_netif_get_ip_info(netif, &ip) == ESP_OK && ip.ip.addr != 0;
  if (!ready) {
    publish(connected);
    return;
  }
  const auto epoch = received.exchange(0);
  if (epoch) {
    RtcCalendar check;
    if (fromEpoch(epoch, check)) {
      model.synced(epoch);
      syncError = ESP_OK;
      rtcError = writeRtc(static_cast<uint64_t>(time(nullptr)));
      ESP_LOGI(TAG, "Internet time synchronized; RTC %s",
               rtcError == ESP_OK ? "saved" : "unavailable");
    }
  }
  const auto now = millis();
  const bool retry = model.state == TimeState::Error;
  bool start = model.tick(now, connected);
  if (requested.exchange(false))
    start = model.request(now, connected) || start;
  if (start) {
    syncError = ESP_OK;
    if (retry) {
      esp_netif_sntp_deinit();
      ntpReady = false;
      server = (server + 1) % 3;
      syncError = configureNtp();
    }
    if (syncError == ESP_OK && ntpReady)
      syncError = esp_netif_sntp_start();
    if (syncError != ESP_OK) {
      model.state = TimeState::Error;
      model.failedAt = now;
    }
  } else if (model.state == TimeState::Error && syncError == ESP_OK)
    syncError = ESP_ERR_TIMEOUT;
  publish(connected);
}
void requestTimeSync() { requested.store(true); }
esp_err_t setTimeZone(unsigned index) {
  if (index >= timeZoneCount)
    return ESP_ERR_INVALID_ARG;
  nvs_handle_t h;
  auto result = nvs_open("dp_time", NVS_READWRITE, &h);
  if (result != ESP_OK)
    return result;
  result = nvs_set_u8(h, "zone", static_cast<unsigned char>(index));
  if (result == ESP_OK)
    result = nvs_commit(h);
  nvs_close(h);
  if (result == ESP_OK) {
    setenv("TZ", timeZones[index].rule, 1);
    tzset();
    zone.store(index);
  }
  return result;
}
TimeSnapshot timeSnapshot() {
  portENTER_CRITICAL(&lock);
  auto s = published;
  portEXIT_CRITICAL(&lock);
  s.zone = zone.load();
  if (s.valid)
    s.epoch = static_cast<uint64_t>(time(nullptr));
  return s;
}
} // namespace dpwatch
