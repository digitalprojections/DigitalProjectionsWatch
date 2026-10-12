#pragma once
#include <cstdint>
namespace dpwatch {
struct RtcCalendar {
  int year = 2000, month = 1, day = 1, hour = 0, minute = 0, second = 0,
      weekday = 0;
};
inline bool leapYear(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
inline int monthDays(int year, int month) {
  const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return month >= 1 && month <= 12
             ? d[month - 1] + (month == 2 && leapYear(year))
             : 0;
}
inline bool validCalendar(const RtcCalendar &d) {
  return d.year >= 2000 && d.year <= 2099 && d.month >= 1 && d.month <= 12 &&
         d.day >= 1 && d.day <= monthDays(d.year, d.month) && d.hour >= 0 &&
         d.hour < 24 && d.minute >= 0 && d.minute < 60 && d.second >= 0 &&
         d.second < 60 && d.weekday >= 0 && d.weekday < 7;
}
inline bool toEpoch(const RtcCalendar &d, uint64_t &epoch) {
  if (!validCalendar(d))
    return false;
  uint64_t days = 0;
  for (int y = 2000; y < d.year; ++y)
    days += leapYear(y) ? 366 : 365;
  for (int m = 1; m < d.month; ++m)
    days += monthDays(d.year, m);
  days += d.day - 1;
  epoch =
      946684800ULL + days * 86400 + d.hour * 3600 + d.minute * 60 + d.second;
  return true;
}
inline bool fromEpoch(uint64_t epoch, RtcCalendar &d) {
  if (epoch < 946684800ULL || epoch >= 4102444800ULL)
    return false;
  auto days = (epoch - 946684800ULL) / 86400;
  const auto seconds = epoch % 86400;
  d = {};
  d.weekday = static_cast<int>((epoch / 86400 + 4) % 7);
  while (days >= static_cast<unsigned>(leapYear(d.year) ? 366 : 365)) {
    days -= leapYear(d.year) ? 366 : 365;
    ++d.year;
  }
  while (days >= static_cast<unsigned>(monthDays(d.year, d.month))) {
    days -= monthDays(d.year, d.month);
    ++d.month;
  }
  d.day = static_cast<int>(days) + 1;
  d.hour = static_cast<int>(seconds / 3600);
  d.minute = static_cast<int>(seconds / 60 % 60);
  d.second = static_cast<int>(seconds % 60);
  return true;
}
inline unsigned char bcd(int n) {
  return static_cast<unsigned char>((n / 10) * 16 + n % 10);
}
inline int unbcd(unsigned char n) {
  return (n & 15) < 10 && (n >> 4) < 10 ? (n >> 4) * 10 + (n & 15) : -1;
}
inline bool encodeRtc(const RtcCalendar &d, unsigned char *b) {
  if (!validCalendar(d))
    return false;
  b[0] = bcd(d.second);
  b[1] = bcd(d.minute);
  b[2] = bcd(d.hour);
  b[3] = bcd(d.day);
  b[4] = static_cast<unsigned char>(d.weekday);
  b[5] = bcd(d.month);
  b[6] = bcd(d.year - 2000);
  return true;
}
inline bool decodeRtc(const unsigned char *b, RtcCalendar &d) {
  if (b[0] & 0x80)
    return false;
  d = {2000 + unbcd(b[6]),
       unbcd(b[5] & 0x1f),
       unbcd(b[3] & 0x3f),
       unbcd(b[2] & 0x3f),
       unbcd(b[1] & 0x7f),
       unbcd(b[0] & 0x7f),
       b[4] & 7};
  return validCalendar(d);
}
enum class TimeState : unsigned char {
  WaitingWifi,
  Loading,
  Success,
  Error,
  Unavailable
};
enum class TimeSource : unsigned char { None, Rtc, Internet };
struct TimeModel {
  TimeState state = TimeState::WaitingWifi;
  TimeSource source = TimeSource::None;
  bool valid = false, online = false;
  uint32_t started = 0, failedAt = 0;
  uint64_t lastSync = 0;
  void restored() {
    valid = true;
    source = TimeSource::Rtc;
  }
  bool request(uint32_t now, bool connected) {
    if (!connected || state == TimeState::Loading)
      return false;
    started = now;
    state = TimeState::Loading;
    return true;
  }
  bool tick(uint32_t now, bool connected) {
    const bool newly = connected && !online;
    online = connected;
    if (!online) {
      state = TimeState::WaitingWifi;
      return false;
    }
    if (state == TimeState::Loading && uint32_t(now - started) >= 30000) {
      state = TimeState::Error;
      failedAt = now;
      return false;
    }
    if (newly ||
        (state == TimeState::Error && uint32_t(now - failedAt) >= 60000))
      return request(now, true);
    return false;
  }
  void synced(uint64_t epoch) {
    RtcCalendar d;
    if (!fromEpoch(epoch, d))
      return;
    valid = true;
    source = TimeSource::Internet;
    lastSync = epoch;
    state = TimeState::Success;
  }
};
struct TimeZone {
  const char *name;
  const char *rule;
};
inline constexpr TimeZone timeZones[] = {
    {"Japan (Tokyo)", "JST-9"},
    {"China (Shanghai)", "CST-8"},
    {"UTC", "UTC0"},
    {"India (Kolkata)", "IST-5:30"},
    {"UAE (Dubai)", "GST-4"},
    {"UK (London)", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Central Europe", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"US Eastern (New York)", "EST5EDT,M3.2.0,M11.1.0"},
    {"US Pacific (Los Angeles)", "PST8PDT,M3.2.0,M11.1.0"},
    {"Australia (Sydney)", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"New Zealand (Auckland)", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
    {"Uzbekistan (Tashkent)", "UZT-5"},
    {"Turkey (Istanbul)", "TRT-3"},
    {"Thailand (Bangkok)", "ICT-7"},
    {"Brazil (Sao Paulo)", "BRT3"}};
inline constexpr unsigned timeZoneCount =
    sizeof(timeZones) / sizeof(timeZones[0]);
} // namespace dpwatch
