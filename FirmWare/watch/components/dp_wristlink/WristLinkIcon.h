#pragma once
#include <array>
#include "lvgl.h"
namespace wristlink {
inline constexpr auto iconPixels=[](){std::array<uint16_t,112*112> a{};for(int y=0;y<112;++y)for(int x=0;x<112;++x){bool face=x>28&&x<84&&y>30&&y<82;bool band=x>43&&x<69&&y>10&&y<102;bool wings=y>52&&y<60&&x>36&&x<76;bool nose=x>52&&x<60&&y>38&&y<73;a[y*112+x]=(wings||nose)?0xffff:(face?0x1925:(band?0x07ff:0x10a3));}return a;}();
inline const lv_image_dsc_t icon=[](){lv_image_dsc_t i{};i.header.magic=LV_IMAGE_HEADER_MAGIC;i.header.cf=LV_COLOR_FORMAT_RGB565;i.header.w=112;i.header.h=112;i.header.stride=224;i.data_size=sizeof(iconPixels);i.data=reinterpret_cast<const uint8_t*>(iconPixels.data());return i;}();
}
