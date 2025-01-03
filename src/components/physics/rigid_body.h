#pragma once
#include <rendering/types.h>
#include <core/component.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

namespace JPH {
    class Shape;
}

struct RigidBody : public Component {
    JPH::BodyID body_id;
    const JPH::Shape* shape;

    Vec3 linear_velocity;

    void initialize() override;
    void set_linear_velocity(Vec3 p_linear_velocity);
    Vec3 get_linear_velocity() const;
    void update(double delta) override;
    void cleanup() override;
};