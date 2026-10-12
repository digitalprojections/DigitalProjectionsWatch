#pragma once
#include <array>
#include <cstdint>
namespace dpwatch {
enum class Direction { None, Left, Right, Up, Down };
class Game2048 {
    uint32_t randomState = 1;
    uint32_t random() {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        return randomState;
    }
    static int index(Direction d, int line, int pos) {
        switch (d) {
        case Direction::Left:
            return line * 4 + pos;
        case Direction::Right:
            return line * 4 + 3 - pos;
        case Direction::Up:
            return pos * 4 + line;
        case Direction::Down:
            return (3 - pos) * 4 + line;
        default:
            return 0;
        }
    }

  public:
    std::array<uint32_t, 16> cells{};
    uint64_t score = 0;
    bool spawn(uint32_t choice, uint32_t value) {
        std::array<int, 16> empty{};
        unsigned count = 0;
        for (int i = 0; i < 16; ++i)
            if (cells[i] == 0)
                empty[count++] = i;
        if (!count)
            return false;
        cells[empty[choice % count]] = (value % 10 == 0) ? 4u : 2u;
        return true;
    }
    void reset(uint32_t seed) {
        cells.fill(0);
        score = 0;
        randomState = seed ? seed : 1;
        for (int i = 0; i < 2; ++i) {
            auto a = random();
            auto b = random();
            spawn(a, b);
        }
    }
    bool slide(Direction d) {
        if (d == Direction::None)
            return false;
        const auto before = cells;
        for (int line = 0; line < 4; ++line) {
            std::array<uint32_t, 4> values{}, output{};
            int n = 0, out = 0;
            for (int p = 0; p < 4; ++p) {
                auto v = cells[index(d, line, p)];
                if (v)
                    values[n++] = v;
            }
            for (int p = 0; p < n; ++p) {
                auto v = values[p];
                if (p + 1 < n && v == values[p + 1] && v <= UINT32_MAX / 2) {
                    v *= 2;
                    ++p;
                    score += v;
                }
                output[out++] = v;
            }
            for (int p = 0; p < 4; ++p)
                cells[index(d, line, p)] = output[p];
        }
        return cells != before;
    }
    bool move(Direction d) {
        if (!slide(d))
            return false;
        auto a = random();
        auto b = random();
        spawn(a, b);
        return true;
    }
    bool won() const {
        for (auto v : cells)
            if (v >= 2048)
                return true;
        return false;
    }
    bool canMove() const {
        for (int i = 0; i < 16; ++i) {
            if (!cells[i])
                return true;
            if (i % 4 < 3 && cells[i] == cells[i + 1])
                return true;
            if (i < 12 && cells[i] == cells[i + 4])
                return true;
        }
        return false;
    }
};
} // namespace dpwatch
