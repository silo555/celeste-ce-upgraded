#pragma once

// Edge-triggered controls shared by the runtime and host regression tests.
struct RunControls {
    enum Action { none, pause_changed, reset };
    bool paused = false;
    bool reset_held = true;
    bool pause_held = true;

    Action sample(bool reset_down, bool pause_down, bool can_pause) {
        bool reset_pressed = reset_down && !reset_held;
        bool pause_pressed = pause_down && !pause_held;
        reset_held = reset_down;
        pause_held = pause_down;
        if(reset_pressed) {
            paused = false;
            return reset;
        }
        if(pause_pressed && can_pause) {
            paused = !paused;
            return pause_changed;
        }
        return none;
    }
};
