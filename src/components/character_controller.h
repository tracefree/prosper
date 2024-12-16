#pragma once

#include <core/component.h>
#include <SDL3/SDL.h>

struct CharacterController : public Component {
    bool enabled { true };
    Vec3 velocity;

    void update(double delta) override;
    void process_input(SDL_Event& event) override;
    static std::string get_name();
};