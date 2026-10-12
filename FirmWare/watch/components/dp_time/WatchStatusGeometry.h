#pragma once
namespace dpwatch {
inline constexpr int statusBarHeight = 48;
inline constexpr int statusSideInset = 40;
inline constexpr int statusIconSize = 24;
inline bool statusBoundsSafe(int x1, int y1, int x2, int y2) {
  return x1 >= statusSideInset && x2 < 410 - statusSideInset && y1 >= 10 &&
         y2 < statusBarHeight;
}
} // namespace dpwatch
