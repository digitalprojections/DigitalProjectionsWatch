#pragma once
#include "CalibrationService.h"
namespace dpwatch {
struct MotionSnapshot {
    MotionSample sample{};
    CalibrationState state = CalibrationState::Unavailable;
    unsigned timestamp = 0;
    unsigned generation = 0;
    unsigned ageMs = 0;
    bool bootPressed = false;
};
bool startMotionService();
MotionSnapshot motionSnapshot();
void requestMotionCalibration();
} // namespace dpwatch
