#include <components/camera.h>

#include <input.h>
#include <core/node.h>
#include <rendering/renderer.h>
#include <util.h>
#include <physics.h>

#include <SDL3/SDL.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>


extern Renderer gRenderer;

class CastCollector final : public JPH::CastShapeCollector {
    public:
    float available_zoom {1.0f};
    void AddHit(const JPH::ShapeCastResult& in_result) {
        available_zoom = in_result.mFraction;
        ForceEarlyOut();
    }
};

class CameraBodyFilter final : public JPH::BodyFilter {
    public:
    JPH::BodyID* body_to_exclude;
    virtual bool ShouldCollide(const JPH::BodyID& in_body_id) const override {
        if (body_to_exclude == nullptr) {
            return true;
        }
        return (in_body_id != *body_to_exclude);
    }
};

CastCollector cast_collector;
CameraBodyFilter body_filter;

void Camera::update(double delta) {
    if (follow_target != nullptr) {
        node->set_position(Math::interpolate(node->get_position(), follow_target->get_global_transform().position + offset, 0.1f, float(delta)));
    } else {
        const Vec2 movement_input = Input::get_movement_input();
        Vec3 velocity = get_rotation_matrix() * Vec4(movement_input.x, 0.0f, -movement_input.y, 0.0f);
        velocity.y += (Input::is_pressed(SDL_SCANCODE_E) ? 1.0f : 0.0f) + (Input::is_pressed(SDL_SCANCODE_Q) ? -1.0f : 0.0f);
        const Vec3 normalized_velocity = glm::length(velocity) > 0.1 ? glm::normalize(velocity) : Vec3(0.0f);
        speed = Input::is_pressed(SDL_SCANCODE_LSHIFT) ? 7.0f : 3.0f;
        node->move(float(delta) * speed * normalized_velocity);
    }

    zoom = Math::interpolate(zoom, target_zoom, 0.1f, delta);

    JPH::Vec3 position(
        node->get_global_transform().position.x,
        node->get_global_transform().position.y,
        node->get_global_transform().position.z
    );
    JPH::Quat rotation = JPH::Quat::sEulerAngles(JPH::Vec3(pitch, -yaw, 0.0));
    const auto cast_settings = JPH::ShapeCastSettings();
    const auto transform = JPH::Mat44::sTranslation(position);
    const auto shape_cast = JPH::RShapeCast(shape, JPH::Vec3(1.0, 1.0, 1.0), transform, rotation * JPH::Vec3(0.0, 0.0, zoom));
    Physics::physics_system.GetNarrowPhaseQuery().CastShape(
        shape_cast,
        cast_settings,
        shape_cast.mCenterOfMassStart.GetTranslation(),
        cast_collector,
        Physics::physics_system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        Physics::physics_system.GetDefaultLayerFilter(Layers::MOVING),
        body_filter
    );

    float safe_zoom = std::min(zoom, cast_collector.available_zoom * zoom);
    
    if (zoom > safe_zoom) {
        zoom = safe_zoom;
    }
    cast_collector.Reset();
    cast_collector.available_zoom = 1.0f;
}

void Camera::process_input(SDL_Event& event) {
    if (!controls_enabled) { return; }

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        yaw   += (float) event.motion.xrel / 500.0f;
        pitch -= (float) event.motion.yrel / 500.0f;
        pitch  = std::clamp(pitch, -1.57f, 1.57f);
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        float reference_zoom = event.wheel.y > 0.0f ? zoom : target_zoom;
        target_zoom = std::clamp(target_zoom - target_zoom * event.wheel.y * 0.1f, MIN_ZOOM, MAX_ZOOM);
    }
}

Mat4 Camera::get_view_matrix() const {
    Mat4 translation = glm::translate(Mat4(1.0f), node->get_global_transform().position);
    Mat4 rotation = get_rotation_matrix();
    if (follow_target != nullptr) {
        Mat4 distance = glm::translate(Mat4(1.0f), Vec3(0.0, 0.0, zoom));
        return glm::inverse(translation * rotation * distance);
    } else {
        return glm::inverse(translation * rotation);
    }
}

Mat4 Camera::get_rotation_matrix() const {
    glm::quat pitch_rotation = glm::angleAxis(pitch, Vec3(1.0f, 0.0f, 0.0f));
    glm::quat yaw_rotation = glm::angleAxis(yaw, Vec3(0.0f, -1.0f, 0.0f));
    return glm::toMat4(yaw_rotation * pitch_rotation);
}

Mat4 Camera::get_horizontal_rotation_matrix() const {
    glm::quat yaw_rotation = glm::angleAxis(-yaw, Vec3(0.0f, 1.0f, 0.0f));
    return glm::toMat4(yaw_rotation);
}

void Camera::initialize() {
    body_filter.body_to_exclude = &body_to_exclude;
    shape = new JPH::SphereShape(0.2f);
}