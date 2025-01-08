#pragma once

#include <rendering/types.h>
#include <core/component.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

namespace JPH {
    class Shape;
}

struct StaticBody : public Component {
    JPH::BodyID body_id;
    const JPH::Shape* shape;

    void initialize() override;
    void cleanup() override;
};