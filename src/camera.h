#pragma once

#include "vk_types.h"
#include <SDL3/SDL.h>

class Camera {
public:
    Vec3 position {Vec3(0.0f, 0.0f, 0.4f)};
    Vec3 velocity {Vec3(0.0f)};

    float pitch {0.0f};
    float yaw {0.0f};

    float speed {1.0f};
    bool controls_enabled { true };

    Mat4 get_view_matrix();
    Mat4 get_rotation_matrix();

    void process_SDL_Event(SDL_Event& event);
    void update(double delta);
};