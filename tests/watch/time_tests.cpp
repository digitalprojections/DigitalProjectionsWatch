#include "TimeModel.h"
#include <cassert>
#include <cstdio>
using namespace dpwatch;
int main() {
  RtcCalendar d{2026, 10, 12, 1, 2, 3, 1};
  uint64_t epoch = 0;
  assert(toEpoch(d, epoch));
  assert(epoch == 1791766923ULL);
  RtcCalendar r;
  assert(fromEpoch(epoch, r));
  assert(r.year == 2026 && r.month == 10 && r.day == 12 && r.hour == 1 &&
         r.second == 3);
  d = {2024, 2, 29, 23, 59, 59, 4};
  assert(toEpoch(d, epoch));
  assert(fromEpoch(epoch + 1, r) && r.month == 3 && r.day == 1 && r.hour == 0);
  d.year = 2025;
  assert(!toEpoch(d, epoch));
  d = {2026, 13, 1, 0, 0, 0, 0};
  assert(!toEpoch(d, epoch));
  assert(!fromEpoch(0, r));
  unsigned char raw[7];
  d = {2026, 10, 12, 1, 2, 3, 1};
  assert(encodeRtc(d, raw));
  assert(raw[0] == 3 && raw[5] == 0x10 && raw[6] == 0x26);
  assert(decodeRtc(raw, r) && r.day == 12);
  raw[0] |= 0x80;
  assert(!decodeRtc(raw, r));
  raw[0] = 0x6a;
  assert(!decodeRtc(raw, r));
  TimeModel m;
  assert(!m.valid && m.state == TimeState::WaitingWifi);
  assert(!m.tick(0, false));
  assert(m.tick(10, true) && m.state == TimeState::Loading);
  assert(!m.request(11, true));
  assert(!m.tick(30009, true));
  assert(!m.tick(30010, true) && m.state == TimeState::Error);
  assert(m.tick(90010, true));
  m.synced(1791766923ULL);
  assert(m.valid && m.source == TimeSource::Internet &&
         m.state == TimeState::Success);
  assert(!m.tick(90011, false) && m.state == TimeState::WaitingWifi && m.valid);
  assert(m.tick(90012, true));
  TimeModel saved;
  saved.restored();
  assert(saved.valid && saved.source == TimeSource::Rtc);
  assert(!saved.request(0, false));
  TimeModel wrap;
  assert(wrap.tick(0xfffffff0U, true));
  assert(!wrap.tick(29984U, true) && wrap.state == TimeState::Error);
  puts("Time tests passed: UTC calendar, leap day, invalid BCD/RTC, sync "
       "states, timeout/retry, offline retention and tick wrap");
}
