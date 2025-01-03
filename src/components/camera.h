#pragma once

#include <math.h>
#include <core/component.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

union SDL_Event;
namespace JPH {
    class SphereShape;
}

class Camera : public Component {
public:
    Vec3 horizontal_velocity {Vec3(0.0f)};
    float vertical_velocity {0.0f};

    const float MIN_ZOOM = 0.5f;
    const float MAX_ZOOM = 5.0f;

    float pitch {0.0f};
    float yaw {0.0f};
    float target_zoom {2.0f};
    float zoom {2.0f};

    float speed {1.0f};
    bool controls_enabled { true };
    bool free_fly { false };
    Node* follow_target;
    Vec3 offset { 0.0f, 1.4f, 0.0f };

    JPH::SphereShape* shape;
    JPH::BodyID body_to_exclude;

    Mat4 get_view_matrix() const;
    Mat4 get_rotation_matrix() const;
    Mat4 get_horizontal_rotation_matrix() const;

    void update(double delta) override;
    void process_input(SDL_Event& event) override;
    void initialize() override;
};