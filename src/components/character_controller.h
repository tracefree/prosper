#pragma once

#include <core/component.h>
#include <SDL3/SDL.h>

namespace JPH {
    class CharacterVirtual;
}

struct CharacterController : public Component {
    bool enabled { true };
    Vec3 target_velocity {};
    JPH::CharacterVirtual* character;

    void update(double delta) override;
    void initialize() override;

    COMPONENT_FACTORY_H(CharacterController)
};