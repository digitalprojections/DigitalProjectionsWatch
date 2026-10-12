#pragma once
namespace dpwatch {
enum class ButtonAction { None, TogglePause, Calibrate, NewGame };
class BootButton {
    bool initialized = false, raw = false, pressed = false, armed = false, held = false,
         pending = false;
    unsigned changed = 0, pressedAt = 0, releasedAt = 0;

  public:
    ButtonAction update(bool input, unsigned now) {
        if (!initialized) {
            initialized = true;
            raw = pressed = input;
            changed = now;
            armed = !input;
            return ButtonAction::None;
        }
        if (input != raw) {
            raw = input;
            changed = now;
        }
        if (raw != pressed && now - changed >= 35) {
            pressed = raw;
            if (pressed) {
                pressedAt = now;
                held = false;
            } else if (!armed) {
                armed = true;
                pending = false;
            } else if (held) {
                held = false;
                pending = false;
            } else if (pending && now - releasedAt <= 360) {
                pending = false;
                return ButtonAction::Calibrate;
            } else {
                pending = true;
                releasedAt = now;
            }
        }
        if (armed && pressed && !held && now - pressedAt >= 2000) {
            held = true;
            pending = false;
            return ButtonAction::NewGame;
        }
        if (armed && !pressed && pending && now - releasedAt >= 360) {
            pending = false;
            return ButtonAction::TogglePause;
        }
        return ButtonAction::None;
    }
};
} // namespace dpwatch
