#pragma once
#include "Game2048.h"
#include <cmath>
namespace dpwatch {
// Accelerations in g; angular velocities in degrees/second.
struct MotionSample {
    float ax, ay, az, gx, gy, gz;
};
enum class MotionState { Loading, Ready, Unavailable, Error };
class MotionControl {
    MotionState current = MotionState::Unavailable;
    unsigned began = 0, stableSince = 0, candidateSince = 0, lastMove = 0, count = 0;
    float sumX = 0, sumY = 0, neutralX = 0, neutralY = 0, previousX = 0, previousY = 0;
    bool stable = false, armed = false;
    Direction candidate = Direction::None;
    static bool valid(const MotionSample &s) {
        return std::isfinite(s.ax) && std::isfinite(s.ay) && std::isfinite(s.az) &&
               std::isfinite(s.gx) && std::isfinite(s.gy) && std::isfinite(s.gz);
    }

  public:
    MotionState state() const { return current; }
    void useNeutral(float x, float y, unsigned now) {
        neutralX = x;
        neutralY = y;
        current = MotionState::Ready;
        armed = false;
        candidate = Direction::None;
        lastMove = now - 300;
    }
    void unavailable() {
        current = MotionState::Unavailable;
        armed = false;
    }
    void begin(unsigned now) {
        current = MotionState::Loading;
        began = now;
        stable = false;
        count = 0;
        sumX = sumY = 0;
        armed = false;
        candidate = Direction::None;
    }
    Direction update(const MotionSample &s, unsigned now) {
        if (current == MotionState::Unavailable || current == MotionState::Error)
            return Direction::None;
        if (!valid(s)) {
            current = MotionState::Error;
            return Direction::None;
        }
        const float gravity = std::sqrt(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
        const float spin = std::sqrt(s.gx * s.gx + s.gy * s.gy + s.gz * s.gz);
        if (current == MotionState::Loading) {
            if (now - began >= 8000) {
                current = MotionState::Error;
                return Direction::None;
            }
            const bool stationary = gravity > .85f && gravity < 1.15f && spin < 5 &&
                                    (!stable || (std::fabs(s.ax - previousX) < .025f &&
                                                 std::fabs(s.ay - previousY) < .025f));
            previousX = s.ax;
            previousY = s.ay;
            if (!stationary) {
                stable = false;
                count = 0;
                sumX = sumY = 0;
                return Direction::None;
            }
            if (!stable) {
                stable = true;
                stableSince = now;
                count = 0;
                sumX = sumY = 0;
            }
            sumX += s.ax;
            sumY += s.ay;
            ++count;
            if (now - stableSince >= 1000 && count >= 30) {
                neutralX = sumX / static_cast<float>(count);
                neutralY = sumY / static_cast<float>(count);
                current = MotionState::Ready;
                armed = true;
                lastMove = now - 300;
            }
            return Direction::None;
        }
        if (gravity < .75f || gravity > 1.25f || spin > 80) {
            candidate = Direction::None;
            return Direction::None;
        }
        float x = (s.ax - neutralX) / gravity, y = (s.ay - neutralY) / gravity;
        if (std::fabs(x) < .12f && std::fabs(y) < .12f) {
            armed = true;
            candidate = Direction::None;
            return Direction::None;
        }
        if (!armed)
            return Direction::None;
        Direction d = Direction::None;
        if (std::fabs(x) > .32f && std::fabs(x) > std::fabs(y) * 1.3f)
            d = x > 0 ? Direction::Right : Direction::Left;
        else if (std::fabs(y) > .32f && std::fabs(y) > std::fabs(x) * 1.3f)
            d = y > 0 ? Direction::Down : Direction::Up;
        if (d == Direction::None) {
            candidate = d;
            return d;
        }
        if (candidate != d) {
            candidate = d;
            candidateSince = now;
            return Direction::None;
        }
        if (now - candidateSince < 100 || now - lastMove < 300)
            return Direction::None;
        lastMove = now;
        armed = false;
        candidate = Direction::None;
        return d;
    }
};
} // namespace dpwatch
