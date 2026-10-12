#pragma once
#include <array>
#include <lvgl.h>
namespace dpwatch {
constexpr auto makeGameIconPixels() {
    std::array<uint16_t, 112 * 112> image{};
    constexpr uint16_t gold = 0xED66, ink = 0x1925;
    for (int y = 0; y < 112; ++y)
        for (int x = 0; x < 112; ++x)
            image[y * 112 + x] = (x < 4 || y < 4 || x >= 108 || y >= 108) ? ink : gold;
    constexpr unsigned char digits[4][7] = {{14, 17, 1, 2, 4, 8, 31},
                                            {14, 17, 19, 21, 25, 17, 14},
                                            {2, 6, 10, 18, 31, 2, 2},
                                            {14, 17, 17, 14, 17, 17, 14}};
    for (int n = 0; n < 4; ++n)
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (digits[n][y] & (1 << (4 - x)))
                    for (int dy = 0; dy < 4; ++dy)
                        for (int dx = 0; dx < 4; ++dx)
                            image[(42 + y * 4 + dy) * 112 + 10 + n * 24 + x * 4 + dx] = ink;
    return image;
}
inline constexpr auto gameIconPixels = makeGameIconPixels();
inline const lv_image_dsc_t gameIcon = [] {
    lv_image_dsc_t icon{};
    icon.header.magic = LV_IMAGE_HEADER_MAGIC;
    icon.header.cf = LV_COLOR_FORMAT_RGB565;
    icon.header.w = 112;
    icon.header.h = 112;
    icon.header.stride = 224;
    icon.data_size = sizeof(gameIconPixels);
    icon.data = reinterpret_cast<const uint8_t *>(gameIconPixels.data());
    return icon;
}();
} // namespace dpwatch
