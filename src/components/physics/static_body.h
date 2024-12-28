#pragma once

#include <rendering/types.h>
#include <core/component.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

struct StaticBody : public Component {
    JPH::BodyID body_id;
    const JPH::Shape* shape;

    void initialize();
    void update(double delta) override;
    void cleanup() override;

    static std::string get_name();
};