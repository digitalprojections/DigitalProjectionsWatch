#pragma once
#include "BootButton.h"
#include "Game2048.h"
#include "SwipeControl.h"
#include <lvgl.h>
namespace dpwatch {
class GameView {
    lv_obj_t *root = nullptr, *tiles[16]{}, *scoreLabel = nullptr, *statusLabel = nullptr;
    SwipeControl swipe;
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
        const char *text = "Swipe to move the tiles";
        if (paused) text = "Paused - press BOOT to resume";
        else if (!game.canMove()) text = "Game over - hold BOOT for a new game";
        else if (game.won()) text = "2048 reached! Keep playing";
        lv_label_set_text(statusLabel, text);
    }

  public:
    Game2048 game;
    // LVGL routes the entire board contact here, including contacts on tiles.
    static void touchEvent(lv_event_t *event) {
        auto *view = static_cast<GameView *>(lv_event_get_user_data(event));
        auto *input = lv_indev_active();
        if (!input) return;
        lv_point_t point;
        lv_indev_get_point(input, &point);
        switch (lv_event_get_code(event)) {
        case LV_EVENT_PRESSED: view->touchDown(point.x, point.y); break;
        case LV_EVENT_RELEASED: view->touchUp(point.x, point.y); break;
        case LV_EVENT_PRESS_LOST: view->swipe.cancel(); break;
        default: break;
        }
    }
    void touchDown(int x, int y) { if (!paused) swipe.press(x, y); }
    void touchUp(int x, int y) {
        const auto direction = swipe.release(x, y);
        if (!paused && direction != Direction::None && game.move(direction)) {
            render();
            status();
        }
    }
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
        lv_obj_remove_flag(board, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE));
        lv_obj_add_flag(board, LV_OBJ_FLAG_PRESS_LOCK);
        lv_obj_add_event_cb(board, touchEvent, LV_EVENT_ALL, this);
        for (int i = 0; i < 16; ++i) {
            auto *tile = lv_obj_create(board);
            lv_obj_set_size(tile, cell, cell);
            lv_obj_set_pos(tile, (i % 4) * (cell + 6), (i / 4) * (cell + 6));
            lv_obj_set_style_border_width(tile, 0, 0);
            lv_obj_remove_flag(tile, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
            tiles[i] = lv_label_create(tile);
            lv_obj_set_style_text_font(tiles[i], &lv_font_montserrat_24, 0);
            lv_obj_center(tiles[i]);
        }
        auto *guide = lv_label_create(root);
        lv_obj_set_width(guide, lv_pct(100));
        lv_obj_set_style_text_align(guide, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(guide, lv_color_hex(0xA9BBCF), 0);
        lv_label_set_text(guide, "BOOT: pause / resume\nHold 2 seconds: new game");
        lv_obj_align(guide, LV_ALIGN_BOTTOM_MID, 0, 0);
        paused = false;
        boot = BootButton{};
        swipe.cancel();
        render();
        status();
    }
    void resumeInput(unsigned) {
        boot = BootButton{};
        swipe.cancel();
    }
    bool button(bool pressed, unsigned now) {
        auto action = boot.update(pressed, now);

        if (action == ButtonAction::TogglePause) {
            paused = !paused;
            swipe.cancel();
            status();
        }
        if (action == ButtonAction::NewGame) {
            game.reset(now);
            paused = false;
            swipe.cancel();
            render();
            status();
        }
        return false;
    }
    void close() {
        swipe.cancel();
        if (root)
            lv_obj_delete(root);
        root = nullptr;
    }
};
} // namespace dpwatch
