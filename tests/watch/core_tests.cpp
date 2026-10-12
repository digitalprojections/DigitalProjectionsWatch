#include "BootButton.h"
#include "Game2048.h"
#include "MotionControl.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace dpwatch;
static int checks = 0;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::printf("FAIL line %d: %s\n", __LINE__, #x);                                       \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
static void game_tests() {
    Game2048 g;
    g.cells = {2, 2, 2, 2, 4, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    CHECK(g.slide(Direction::Left));
    CHECK(g.cells[0] == 4 && g.cells[1] == 4 && g.cells[2] == 0);
    CHECK(g.cells[4] == 8);
    CHECK(g.score == 16);
    CHECK(g.slide(Direction::Left));
    CHECK(g.cells[0] == 8);
    CHECK(g.score == 24);
    const auto before = g.cells;
    CHECK(!g.slide(Direction::Left));
    CHECK(g.cells == before);
    CHECK(g.score == 24);
    g.cells = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0};
    g.score = 0;
    CHECK(g.slide(Direction::Up));
    CHECK(g.cells[0] == 2 && g.cells[1] == 2 && g.cells[12] == 0);
    CHECK(g.slide(Direction::Right));
    CHECK(g.cells[3] == 4 && g.cells[0] == 0);
    CHECK(g.score == 4);
    CHECK(g.slide(Direction::Down));
    CHECK(g.cells[15] == 4);
    g.cells = {2, 4, 2, 4, 4, 2, 4, 2, 2, 4, 2, 4, 4, 2, 4, 2};
    CHECK(!g.canMove());
    CHECK(!g.spawn(0, 0));
    g.cells[0] = 0;
    CHECK(g.canMove());
    CHECK(g.spawn(0, 0));
    CHECK(g.cells[0] == 4);
    g.cells[0] = 2048;
    CHECK(g.won());
    CHECK(!g.canMove());
    g.reset(123);
    int occupied = 0;
    for (auto n : g.cells)
        occupied += n != 0;
    CHECK(occupied == 2);
    CHECK(g.score == 0);
    Game2048 same;
    same.reset(123);
    CHECK(same.cells == g.cells);
    // Invalid/no-op input must never add a random tile.
    const auto saved = g.cells;
    CHECK(!g.move(Direction::None));
    CHECK(g.cells == saved);
    // Each valid move preserves tile mass except for exactly one new 2 or 4.
    for (unsigned seed = 1; seed < 100; ++seed) {
        g.reset(seed);
        for (int i = 0; i < 100; ++i) {
            unsigned a = 0, b = 0;
            for (auto n : g.cells)
                a += n;
            bool changed = g.move(static_cast<Direction>(1 + (i % 4)));
            for (auto n : g.cells)
                b += n;
            CHECK(changed ? (b - a == 2 || b - a == 4) : a == b);
        }
    }
}
static MotionSample flat() { return {0, 0, 1, 0, 0, 0}; }
static void calibrate(MotionControl &m, unsigned start = 0) {
    m.begin(start);
    for (unsigned elapsed = 0; elapsed <= 1200; elapsed += 20)
        m.update(flat(), start + elapsed);
    CHECK(m.state() == MotionState::Ready);
}
static void motion_tests() {
    MotionControl m;
    calibrate(m);
    MotionSample s = flat();
    s.ax = .5f;
    s.az = .866f;
    CHECK(m.update(s, 1300) == Direction::None);
    CHECK(m.update(s, 1420) == Direction::Right);
    CHECK(m.update(s, 2000) == Direction::None); // Held tilt cannot repeat.
    CHECK(m.update(flat(), 2020) == Direction::None);
    CHECK(m.update(s, 2040) == Direction::None);
    CHECK(m.update(s, 2160) == Direction::Right);
    s = flat();
    s.ay = -.5f;
    s.az = .866f;
    m.update(flat(), 2500);
    CHECK(m.update(s, 2520) == Direction::None);
    CHECK(m.update(s, 2640) == Direction::Up);
    s = flat();
    s.ax = .5f;
    s.ay = .5f;
    s.az = .707f;
    m.update(flat(), 3000);
    CHECK(m.update(s, 3020) == Direction::None);
    CHECK(m.update(s, 3200) == Direction::None); // Ambiguous diagonal.
    s = flat();
    s.ax = .5f;
    s.gx = 200;
    CHECK(m.update(s, 3500) == Direction::None);
    s = flat();
    s.ax = NAN;
    CHECK(m.update(s, 4000) == Direction::None);
    CHECK(m.state() == MotionState::Error);
    m.begin(5000);
    m.update(flat(), 14000);
    CHECK(m.state() == MotionState::Error); // Bounded calibration.
    m.begin(0);
    for (unsigned t = 0; t < 9000; t += 20) {
        s = flat();
        s.gx = 100;
        m.update(s, t);
    }
    CHECK(m.state() == MotionState::Error);
    // Non-level but stationary posture is a valid neutral orientation.
    s = {.2f, 0, .9799f, 0, 0, 0};
    m.begin(0);
    for (unsigned t = 0; t <= 1200; t += 20)
        m.update(s, t);
    CHECK(m.state() == MotionState::Ready);
    CHECK(m.update(s, 1500) == Direction::None);
    calibrate(m, 0xFFFFFF00u); // Unsigned elapsed time must tolerate millis wrap.
}
static void button_tests() {
    BootButton b;
    CHECK(b.update(false, 0) == ButtonAction::None);
    CHECK(b.update(true, 100) == ButtonAction::None);
    CHECK(b.update(false, 110) == ButtonAction::None);
    CHECK(b.update(false, 160) == ButtonAction::None); // Bounce ignored.
    b.update(true, 200);
    b.update(true, 240);
    b.update(false, 300);
    b.update(false, 340);
    CHECK(b.update(false, 700) == ButtonAction::TogglePause);
    CHECK(b.update(false, 800) == ButtonAction::None);
    b.update(true, 1000);
    b.update(true, 1040);
    b.update(false, 1100);
    b.update(false, 1140);
    b.update(true, 1200);
    b.update(true, 1240);
    b.update(false, 1300);
    CHECK(b.update(false, 1340) == ButtonAction::Calibrate);
    CHECK(b.update(false, 1800) == ButtonAction::None);
    b.update(true, 2000);
    b.update(true, 2040);
    CHECK(b.update(true, 4050) == ButtonAction::NewGame);
    CHECK(b.update(true, 5000) == ButtonAction::None);
    b.update(false, 5100);
    b.update(false, 5140);
    CHECK(b.update(false, 5500) == ButtonAction::None);
    BootButton held;
    CHECK(held.update(true, 0) == ButtonAction::None);
    CHECK(held.update(true, 3000) == ButtonAction::None);
    held.update(false, 3100);
    held.update(false, 3140);
    held.update(true, 3200);
    held.update(true, 3240);
    CHECK(held.update(true, 5300) == ButtonAction::NewGame);
}
int main() {
    game_tests();
    motion_tests();
    button_tests();
    std::printf("Watch 2048: %d checks passed\n", checks);
}
