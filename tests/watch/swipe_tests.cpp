#include "SwipeControl.h"
#include <cassert>
#include <cstdio>
using namespace dpwatch;
int main() {
 SwipeControl s;
 assert(s.release(100,100)==Direction::None);
 s.press(200,200); assert(s.release(100,205)==Direction::Left);
 s.press(200,200); assert(s.release(300,195)==Direction::Right);
 s.press(200,200); assert(s.release(205,100)==Direction::Up);
 s.press(200,200); assert(s.release(195,300)==Direction::Down);
 s.press(200,200); assert(s.release(220,205)==Direction::None);
 s.press(200,200); assert(s.release(250,250)==Direction::None);
 s.press(200,200); s.cancel(); assert(s.release(100,200)==Direction::None);
 s.press(200,200); assert(s.release(100,200)==Direction::Left);
 assert(s.release(90,200)==Direction::None);
 s.press(100,100); assert(s.release(132,100)==Direction::Right);
 s.press(100,100); assert(s.release(131,100)==Direction::None);
 std::puts("Swipe tests: four directions, tap, diagonal, cancellation, threshold and one move per contact passed.");
}
