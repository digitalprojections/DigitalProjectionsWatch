#pragma once
#include "Game2048.h"
namespace dpwatch {
// Classify one completed contact. Short taps and ambiguous diagonals do not move.
class SwipeControl {
    int startX = 0, startY = 0;
    bool active = false;
public:
    void press(int x, int y) { startX = x; startY = y; active = true; }
    void cancel() { active = false; }
    Direction release(int x, int y) {
        if (!active) return Direction::None;
        active = false;
        const int dx = x - startX, dy = y - startY;
        const int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
        if (ax >= 32 && ax * 2 >= ay * 3)
            return dx < 0 ? Direction::Left : Direction::Right;
        if (ay >= 32 && ay * 2 >= ax * 3)
            return dy < 0 ? Direction::Up : Direction::Down;
        return Direction::None;
    }
};
}
