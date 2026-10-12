#pragma once
#include "MotionControl.h"
#include <array>
#include <cstring>
namespace dpwatch {
enum class CalibrationState { Needed, Loading, Ready, Unavailable, Error };
struct CalibrationProfile {
    uint32_t schema = 1, generation = 0;
    std::array<float, 6> bias{};
    uint32_t checksum = 0;
    uint32_t digest() const {
        uint32_t hash = 2166136261u;
        auto add = [&](const void *ptr, unsigned count) {
            auto *p = static_cast<const unsigned char *>(ptr);
            for (unsigned i = 0; i < count; ++i) {
                hash ^= p[i];
                hash *= 16777619u;
            }
        };
        add(&schema, sizeof(schema));
        add(&generation, sizeof(generation));
        add(bias.data(), static_cast<unsigned>(sizeof(float) * bias.size()));
        return hash;
    }
    bool valid() const {
        if (schema != 1 || generation == 0 || checksum != digest())
            return false;
        for (unsigned i = 0; i < 6; ++i)
            if (!std::isfinite(bias[i]) || std::fabs(bias[i]) > (i < 3 ? .2f : 50.f))
                return false;
        return true;
    }
};
class CalibrationStore {
  public:
    virtual ~CalibrationStore() = default;
    virtual bool load(CalibrationProfile &) = 0;
    virtual bool save(const CalibrationProfile &) = 0;
};
class CalibrationService {
    CalibrationStore &storage;
    CalibrationProfile saved{};
    bool savedValid = false;
    CalibrationState current = CalibrationState::Needed;
    unsigned started = 0, lastSample = 0, stableSince = 0, count = 0;
    std::array<float, 6> sum{}, lo{}, hi{};
    static std::array<float, 6> values(const MotionSample &s) {
        return {s.ax, s.ay, s.az, s.gx, s.gy, s.gz};
    }
    void resetWindow(const std::array<float, 6> &v, unsigned now) {
        sum = v;
        lo = v;
        hi = v;
        count = 1;
        stableSince = now;
    }

  public:
    explicit CalibrationService(CalibrationStore &store) : storage(store) {
        savedValid = storage.load(saved) && saved.valid();
        if (savedValid)
            current = CalibrationState::Ready;
    }
    CalibrationState state() const { return current; }
    const CalibrationProfile &profile() const { return saved; }
    bool hasProfile() const { return savedValid; }
    void start(unsigned now) {
        current = CalibrationState::Loading;
        started = lastSample = now;
        count = 0;
        sum.fill(0);
    }
    void tick(unsigned now) {
        if (current == CalibrationState::Loading && now - started >= 8000) {
            current = CalibrationState::Error;
            return;
        }
        if ((current == CalibrationState::Loading || current == CalibrationState::Ready) &&
            now - lastSample > 500)
            current = CalibrationState::Unavailable;
    }
    void sample(const MotionSample &s, unsigned now) {
        const auto v = values(s);
        for (auto n : v)
            if (!std::isfinite(n)) {
                current = CalibrationState::Error;
                return;
            }
        lastSample = now;
        if (current == CalibrationState::Unavailable) {
            current = savedValid ? CalibrationState::Ready : CalibrationState::Needed;
        }
        if (current != CalibrationState::Loading)
            return;
        if (now - started >= 8000) {
            current = CalibrationState::Error;
            return;
        }
        // A single stationary, flat pose estimates biases. It is not a six-face scale calibration.
        if (std::fabs(s.ax) > .1f || std::fabs(s.ay) > .1f ||
            std::fabs(std::fabs(s.az) - 1) > .15f || std::fabs(s.gx) > 30 || std::fabs(s.gy) > 30 ||
            std::fabs(s.gz) > 30) {
            count = 0;
            return;
        }
        if (!count) {
            resetWindow(v, now);
            return;
        }
        bool stable = true;
        for (unsigned i = 0; i < 6; ++i) {
            lo[i] = std::fmin(lo[i], v[i]);
            hi[i] = std::fmax(hi[i], v[i]);
            if (hi[i] - lo[i] > (i < 3 ? .025f : .5f))
                stable = false;
        }
        if (!stable) {
            resetWindow(v, now);
            return;
        }
        for (unsigned i = 0; i < 6; ++i)
            sum[i] += v[i];
        ++count;
        if (now - stableSince < 1000 || count < 30)
            return;
        CalibrationProfile next;
        next.generation = savedValid ? saved.generation + 1 : 1;
        for (unsigned i = 0; i < 6; ++i)
            next.bias[i] = sum[i] / static_cast<float>(count);
        next.bias[2] -= next.bias[2] >= 0 ? 1.f : -1.f;
        next.checksum = next.digest();
        if (!next.valid() || !storage.save(next)) {
            current = CalibrationState::Error;
            return;
        }
        saved = next;
        savedValid = true;
        current = CalibrationState::Ready;
    }
    MotionSample correct(const MotionSample &s) const {
        if (!savedValid)
            return s;
        return {s.ax - saved.bias[0], s.ay - saved.bias[1], s.az - saved.bias[2],
                s.gx - saved.bias[3], s.gy - saved.bias[4], s.gz - saved.bias[5]};
    }
};
} // namespace dpwatch
