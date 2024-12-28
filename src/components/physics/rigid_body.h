#pragma once
#include <rendering/types.h>
#include <core/component.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

struct RigidBody : public Component {
    JPH::BodyID body_id;
    const JPH::Shape* shape;

    Vec3 linear_velocity;

    void initialize();
    void set_linear_velocity(Vec3 p_linear_velocity);
    Vec3 get_linear_velocity();
    void update(double delta) override;
    void cleanup() override;

    static std::string get_name();
};