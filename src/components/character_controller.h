#pragma once

#include <core/component.h>
#include <SDL3/SDL.h>

namespace JPH {
    class CharacterVirtual;
}

struct CharacterController : public Component {
    bool enabled { true };
    Vec3 target_velocity;
    JPH::CharacterVirtual* character;

    void update(double delta) override;
    void process_input(SDL_Event& event) override;
    void initialize() override;
    void cleanup() override;

    COMPONENT_FACTORY_H(CharacterController)
};