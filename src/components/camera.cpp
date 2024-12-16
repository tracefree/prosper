#include <components/camera.h>

#include <input.h>
#include <core/node.h>
#include <rendering/renderer.h>
#include <util.h>


extern Renderer gRenderer;

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
}

void Camera::process_input(SDL_Event& event) {
    if (!controls_enabled) { return; }

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        yaw   += (float) event.motion.xrel / 500.0f;
        pitch -= (float) event.motion.yrel / 500.0f;
        pitch  = std::clamp(pitch, -1.57f, 1.57f);
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        target_zoom = std::clamp(target_zoom - target_zoom * event.wheel.y * 0.1f, MIN_ZOOM, MAX_ZOOM);
    }
}

Mat4 Camera::get_view_matrix() {
    Mat4 translation = glm::translate(Mat4(1.0f), node->get_global_transform().position);
    Mat4 rotation = get_rotation_matrix();
    if (follow_target != nullptr) {
        Mat4 distance = glm::translate(Mat4(1.0f), Vec3(0.0, 0.0, zoom));
        return glm::inverse(translation * rotation * distance);
    } else {
        return glm::inverse(translation * rotation);
    }
}

Mat4 Camera::get_rotation_matrix() {
    glm::quat pitch_rotation = glm::angleAxis(pitch, Vec3(1.0f, 0.0f, 0.0f));
    glm::quat yaw_rotation = glm::angleAxis(yaw, Vec3(0.0f, -1.0f, 0.0f));
    return glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);
}

Mat4 Camera::get_horizontal_rotation_matrix() {
    glm::quat yaw_rotation = glm::angleAxis(-yaw, Vec3(0.0f, 1.0f, 0.0f));
    return glm::toMat4(yaw_rotation);
}

std::string Camera::get_name() {
    return "Camera";
}