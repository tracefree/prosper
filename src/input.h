#pragma once

#include <util.h>
#include <math.h>
#include <unordered_map>
#include <SDL3/SDL.h>

class Input {
public:
    static const bool* keyboard_state;

    static bool is_pressed(SDL_Scancode keycode);
    static Vec2 get_movement_input();

    static void process_event(SDL_Event& event);
    static void initialize();
};