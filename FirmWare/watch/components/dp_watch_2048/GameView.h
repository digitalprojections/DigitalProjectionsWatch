#pragma once
#include "BootButton.h"
#include "CalibrationService.h"
#include "Game2048.h"
#include "MotionControl.h"
#include <lvgl.h>
namespace dpwatch {
class GameView {
    lv_obj_t *root = nullptr, *tiles[16]{}, *scoreLabel = nullptr, *statusLabel = nullptr;
    CalibrationState sharedState = CalibrationState::Needed;
    bool paused = false;
    BootButton boot;
    void render() {
        lv_label_set_text_fmt(scoreLabel, "2048   |   %llu",
                              static_cast<unsigned long long>(game.score));
        static const uint32_t colors[] = {0x253146, 0xE4DDCE, 0xD8CEB3, 0xF1AD62,
                                          0xEB8958, 0xE76D58, 0xD8515E, 0xE1BC52,
                                          0xD2AA37, 0xBF922F, 0xA77D2B, 0x8E672A};
        for (int i = 0; i < 16; ++i) {
            auto value = game.cells[i];
            unsigned power = 0;
            for (auto n = value; n > 1; n >>= 1)
                ++power;
            lv_obj_set_style_bg_color(lv_obj_get_parent(tiles[i]),
                                      lv_color_hex(colors[power > 11 ? 11 : power]), 0);
            lv_obj_set_style_text_color(tiles[i], lv_color_hex(power < 3 ? 0x253146 : 0xFFFFFF), 0);
            if (value)
                lv_label_set_text_fmt(tiles[i], "%lu", static_cast<unsigned long>(value));
            else
                lv_label_set_text(tiles[i], "");
        }
    }
    void status() {
        if (!root)
            return;
        const char *text = "Double-press BOOT to calibrate";
        if (sharedState == CalibrationState::Loading)
            text = "Keep flat and still - calibrating";
        else if (sharedState == CalibrationState::Unavailable)
            text = "Motion sensor unavailable";
        else if (sharedState == CalibrationState::Error)
            text = "Calibration failed - double-press BOOT";
        else if (sharedState == CalibrationState::Ready) {
            if (paused)
                text = "Paused - press BOOT to resume";
            else if (!game.canMove())
                text = "Game over - hold BOOT for a new game";
            else
                text =
                    game.won() ? "2048 reached! Keep playing" : "Tilt to move; return to neutral";
        }
        lv_label_set_text(statusLabel, text);
    }

  public:
    Game2048 game;
    MotionControl motion;
    void open(lv_obj_t *parent, unsigned seed) {
        if (game.cells == std::array<uint32_t, 16>{})
            game.reset(seed);
        root = lv_obj_create(parent);
        lv_obj_set_size(root, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_color(root, lv_color_hex(0x101827), 0);
        lv_obj_set_style_border_width(root, 0, 0);
        lv_obj_set_style_pad_all(root, 12, 0);
        lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
        scoreLabel = lv_label_create(root);
        lv_obj_set_style_text_color(scoreLabel, lv_color_hex(0xFFFFFF), 0);
        lv_obj_align(scoreLabel, LV_ALIGN_TOP_MID, 0, 0);
        statusLabel = lv_label_create(root);
        lv_obj_set_width(statusLabel, lv_pct(100));
        lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xA9BBCF), 0);
        lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(statusLabel, LV_ALIGN_TOP_MID, 0, 32);
        lv_obj_update_layout(root);
        int width = lv_obj_get_content_width(root), height = lv_obj_get_content_height(root);
        int side = width < height - 132 ? width : height - 132;
        int cell = (side - 18) / 4;
        auto *board = lv_obj_create(root);
        lv_obj_set_size(board, side, side);
        lv_obj_align(board, LV_ALIGN_TOP_MID, 0, 72);
        lv_obj_set_style_pad_all(board, 0, 0);
        lv_obj_set_style_border_width(board, 0, 0);
        lv_obj_set_style_bg_color(board, lv_color_hex(0x101827), 0);
        lv_obj_remove_flag(board, LV_OBJ_FLAG_SCROLLABLE);
        for (int i = 0; i < 16; ++i) {
            auto *tile = lv_obj_create(board);
            lv_obj_set_size(tile, cell, cell);
            lv_obj_set_pos(tile, (i % 4) * (cell + 6), (i / 4) * (cell + 6));
            lv_obj_set_style_border_width(tile, 0, 0);
            lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
            tiles[i] = lv_label_create(tile);
            lv_obj_set_style_text_font(tiles[i], &lv_font_montserrat_24, 0);
            lv_obj_center(tiles[i]);
        }
        auto *guide = lv_label_create(root);
        lv_obj_set_width(guide, lv_pct(100));
        lv_obj_set_style_text_align(guide, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(guide, lv_color_hex(0xA9BBCF), 0);
        lv_label_set_text(guide, "BOOT: pause / resume\nTwo presses: calibrate  |  Hold: new game");
        lv_obj_align(guide, LV_ALIGN_BOTTOM_MID, 0, 0);
        paused = false;
        boot = BootButton{};
        if (sharedState == CalibrationState::Ready)
            motion.useNeutral(0, 0, lv_tick_get());
        render();
        status();
    }
    void resumeInput(unsigned now) {
        boot = BootButton{};
        if (sharedState == CalibrationState::Ready) motion.useNeutral(0, 0, now);
    }
    void calibrationState(CalibrationState state, unsigned now) {
        if (state == sharedState)
            return;
        sharedState = state;
        if (state == CalibrationState::Ready)
            motion.useNeutral(0, 0, now);
        else
            motion.unavailable();
        status();
    }
    void feed(const MotionSample &sample, unsigned now) {
        if (paused || sharedState != CalibrationState::Ready)
            return;
        const auto direction = motion.update(sample, now);
        if (direction != Direction::None && game.move(direction)) {
            render();
            status();
        }
    }
    bool button(bool pressed, unsigned now) {
        auto action = boot.update(pressed, now);
        if (action == ButtonAction::Calibrate)
            return sharedState != CalibrationState::Loading;
        if (action == ButtonAction::TogglePause) {
            paused = !paused;
            if (!paused && sharedState == CalibrationState::Ready)
                motion.useNeutral(0, 0, now);
            status();
        }
        if (action == ButtonAction::NewGame) {
            game.reset(now);
            paused = false;
            if (sharedState == CalibrationState::Ready)
                motion.useNeutral(0, 0, now);
            render();
            status();
        }
        return false;
    }
    void close() {
        if (root)
            lv_obj_delete(root);
        root = nullptr;
    }
};
} // namespace dpwatch
