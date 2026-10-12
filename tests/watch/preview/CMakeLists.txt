cmake_minimum_required(VERSION 3.20)
project(Watch2048Preview LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
file(GLOB_RECURSE LV_SOURCES "${VENDOR}/lvgl/src/*.c")
list(FILTER LV_SOURCES EXCLUDE REGEX "/demos/")
add_library(lvgl STATIC ${LV_SOURCES})
target_include_directories(lvgl PUBLIC "${VENDOR}/lvgl" "${VENDOR}")
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
add_executable(preview preview.cpp)
target_include_directories(preview PRIVATE ../../../FirmWare/watch/components/dp_watch_2048 ../../../FirmWare/watch/components/dp_motion/include)
target_link_libraries(preview PRIVATE lvgl)


target_compile_options(preview PRIVATE /UNDEBUG)
