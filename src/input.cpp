#include <input.h>

const bool* Input::keyboard_state;


void Input::initialize() {
    if (keyboard_state != nullptr) {
        print("Warning: Input system already initialized");
        return;
    }
    
    keyboard_state = SDL_GetKeyboardState(nullptr);

    if (keyboard_state == nullptr) {
        print("Error: Could not initialize input system!");
    }
}

void Input::process_event(SDL_Event& event) {
    // TODO: Does this need to do anything?
}

bool Input::is_pressed(SDL_Scancode scancode) {
    if (keyboard_state == nullptr) {
        print("Input system not initialized!");
        return false;
    }
    return keyboard_state[scancode];
}

Vec2 Input::get_movement_input() {
    const float x_axis = (is_pressed(SDL_SCANCODE_A) ? -1.0f : 0.0f) + (is_pressed(SDL_SCANCODE_D) ?  1.0f : 0.0f);
    const float y_axis = (is_pressed(SDL_SCANCODE_W) ?  1.0f : 0.0f) + (is_pressed(SDL_SCANCODE_S) ? -1.0f : 0.0f);
    
    const Vec2 movement_input { x_axis, y_axis };
    if (x_axis != 0.0f && y_axis != 0.0f) {
        return glm::normalize(movement_input);
    }
    return movement_input;
}