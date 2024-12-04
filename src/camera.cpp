#include "camera.h"

#include "util.h"


void Camera::update(double delta) {
    Mat4 camera_rotation = get_rotation_matrix();

    Vec3 normalized_velocity = glm::length(velocity) > 0.1 ? glm::normalize(velocity) : Vec3(0.0f);
    position += float(delta) * Vec3(camera_rotation * Vec4(speed * normalized_velocity, 0.0f));
}

void Camera::process_SDL_Event(SDL_Event& event) {
    if (!controls_enabled) { return; }

    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.scancode == SDL_SCANCODE_W) { velocity.z = -1.0; }
        if (event.key.scancode == SDL_SCANCODE_A) { velocity.x = -1.0; }
        if (event.key.scancode == SDL_SCANCODE_S) { velocity.z =  1.0; }
        if (event.key.scancode == SDL_SCANCODE_D) { velocity.x =  1.0; }
        if (event.key.scancode == SDL_SCANCODE_LSHIFT) { speed = 10.0; }
    }

    if (event.type == SDL_EVENT_KEY_UP) {
        if (event.key.scancode == SDL_SCANCODE_W) { velocity.z =  0.0; }
        if (event.key.scancode == SDL_SCANCODE_A) { velocity.x =  0.0; }
        if (event.key.scancode == SDL_SCANCODE_S) { velocity.z =  0.0; }
        if (event.key.scancode == SDL_SCANCODE_D) { velocity.x =  0.0; }
        if (event.key.scancode == SDL_SCANCODE_LSHIFT) { speed =  1.0; }
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        yaw   += (float) event.motion.xrel / 500.0f;
        pitch -= (float) event.motion.yrel / 500.0f;
        pitch  = std::clamp(pitch, -1.57f, 1.57f);
    }
}

Mat4 Camera::get_view_matrix() {
    Mat4 translation = glm::translate(Mat4(1.0f), position);
    Mat4 rotation = get_rotation_matrix();
    return glm::inverse(translation * rotation);
}

Mat4 Camera::get_rotation_matrix() {
    glm::quat pitch_rotation = glm::angleAxis(pitch, Vec3(1.0f, 0.0f, 0.0f));
    glm::quat yaw_rotation = glm::angleAxis(yaw, Vec3(0.0f, -1.0f, 0.0f));
    return glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);
}