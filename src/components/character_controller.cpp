#include <components/character_controller.h>

#include <components/camera.h>
#include <components/animation_player.h>
#include <input.h>
#include <core/node.h>
#include <core/scene_graph.h>

extern SceneGraph scene;
extern std::shared_ptr<AnimationPlayer> animation_player;

void CharacterController::process_input(SDL_Event& event) {

}

void CharacterController::update(double delta) {
    const bool grounded = node->get_global_transform().position.y <= 0.0f;
    if (!grounded) {
        velocity.y -= 9.81f * delta;
        if (animation_player->current_animation != "Air-loop") {
            animation_player->play("Air-loop");
        }
    } else {
        velocity.y = (Input::is_pressed(SDL_SCANCODE_SPACE)) ? 4.0f : 0.0f;
    }
    
    const float speed = Input::is_pressed(SDL_SCANCODE_LSHIFT) ? 5.0f : 2.8f;
    const Vec2 movement_input = enabled ? Input::get_movement_input() : Vec2(0.0f);
    const Vec3 movement_direction = Mat3(scene.camera->get_horizontal_rotation_matrix()) * Vec3(movement_input.x, 0.0f, -movement_input.y);
    const Vec3 target_velocity = movement_direction * speed;
    if (glm::length(target_velocity) > 0.0f) {
        Vec2 current_planar_velocity(velocity.x, velocity.z);
        Vec2 target_planar_velocity(target_velocity.x, target_velocity.z);
        Vec2 new_planar_velocity = Math::interpolate(current_planar_velocity, target_planar_velocity, 0.05f, delta);
        velocity.x = new_planar_velocity.x;
        velocity.z = new_planar_velocity.y;

        glm::quat target_rotation = glm::quatLookAt(-movement_direction, Vec3(0.0f, 1.0f, 0.0f));
        float weight = (1.0f - std::exp(-(float(delta)/0.075f)));
        node->set_rotation(glm::slerp(node->get_rotation(), target_rotation, weight));
    } else if (grounded) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
    }
    
    node->move(velocity * float(delta));

    if (!grounded) return;

    if (std::abs(movement_input.x) > 0 || std::abs(movement_input.y) > 0) {
        if (animation_player->current_animation != "Run-loop" && !Input::is_pressed(SDL_SCANCODE_LSHIFT)) {
            animation_player->play("Run-loop");
        } else if (animation_player->current_animation != "Sprint-loop" && Input::is_pressed(SDL_SCANCODE_LSHIFT)) {
            animation_player->play("Sprint-loop");
        }
    } else if (animation_player->current_animation != "Idle-loop") {
        animation_player->play("Idle-loop");
    }
}

std::string CharacterController::get_name() {
    return "CharacterController";
}