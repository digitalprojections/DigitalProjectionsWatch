#include "CalibrationService.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace dpwatch;
class Store final : public CalibrationStore {
  public:
    bool present = false, fail = false;
    CalibrationProfile saved{};
    bool load(CalibrationProfile &p) override {
        if (!present)
            return false;
        p = saved;
        return true;
    }
    bool save(const CalibrationProfile &p) override {
        if (fail)
            return false;
        saved = p;
        present = true;
        return true;
    }
};
static void finish(CalibrationService &s, unsigned start = 0) {
    s.start(start);
    for (unsigned t = 0; t <= 1200; t += 20)
        s.sample({.02f, -.01f, 1.03f, 1, -2, .5f}, start + t);
    assert(s.state() == CalibrationState::Ready);
}
int main() {
    Store store;
    CalibrationService first(store);
    assert(first.state() == CalibrationState::Needed);
    finish(first);
    MotionSample corrected = first.correct({.02f, -.01f, 1.03f, 1, -2, .5f});
    assert(std::fabs(corrected.ax) < .0001f && std::fabs(corrected.ay) < .0001f &&
           std::fabs(corrected.az - 1) < .0001f);
    assert(std::fabs(corrected.gx) < .0001f && std::fabs(corrected.gy) < .0001f &&
           std::fabs(corrected.gz) < .0001f);
    CalibrationService second(store);
    assert(second.state() == CalibrationState::Ready);
    assert(second.profile().generation == first.profile().generation);
    // Multiple app clients see the same profile after a shared update.
    finish(first, 2000);
    assert(first.profile().generation == 2);
    CalibrationService reboot(store);
    assert(reboot.profile().generation == 2);
    const auto old = store.saved;
    store.fail = true;
    first.start(4000);
    for (unsigned t = 4000; t <= 5200; t += 20)
        first.sample({0, 0, 1, 0, 0, 0}, t);
    assert(first.state() == CalibrationState::Error);
    assert(store.saved.checksum == old.checksum);
    store.fail = false;
    store.saved.checksum ^= 1;
    CalibrationService corrupt(store);
    assert(corrupt.state() == CalibrationState::Needed);
    corrupt.start(0);
    corrupt.tick(8000);
    assert(corrupt.state() == CalibrationState::Error);
    corrupt.start(0);
    for (unsigned t = 0; t < 8100; t += 20)
        corrupt.sample({.5f, 0, .866f, 0, 0, 0}, t);
    assert(corrupt.state() == CalibrationState::Error);
    corrupt.start(0);
    corrupt.sample({NAN, 0, 1, 0, 0, 0}, 10);
    assert(corrupt.state() == CalibrationState::Error);
    corrupt.start(0);
    corrupt.tick(600);
    assert(corrupt.state() == CalibrationState::Unavailable);
    finish(corrupt, 0xFFFFFF00u);
    assert(corrupt.state() == CalibrationState::Ready);
    std::puts("Shared calibration: bias correction, reload, generation, corrupt data, failed save, "
              "timeout, invalid input, missing sensor and timer wrap passed.");
}
