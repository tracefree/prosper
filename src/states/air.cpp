#include <states/air.h>
#include <components/camera.h>
#include <components/character_controller.h>
#include <core/scene_graph.h>
#include <states/idle.h>
#include <physics.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

extern SceneGraph scene;

const float MAX_AIR_MOVE_SPEED = 5.0f;

void State::Air::update(double delta) {
    auto character_controller = actor->get_component<CharacterController>();
    character_controller->target_velocity.y -= 9.81f * float(delta);

    const bool grounded = (character_controller->character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround);
    if (grounded) {
        state_machine->transition_to<Idle>();
        return;
    }

    const Vec2 movement_input = Input::get_movement_input();
    if (glm::length2(movement_input) == 0.0f) {
        return;
    }

    const Vec3 movement_direction = Mat3(scene.camera->get_component<Camera>()->get_horizontal_rotation_matrix()) * Vec3(movement_input.x, 0.0f, -movement_input.y);
    glm::quat target_rotation = glm::quatLookAt(-movement_direction, Vec3(0.0f, 1.0f, 0.0f));
    float weight = (1.0f - std::exp(-(float(delta)/0.025f)));
    actor->set_rotation(glm::slerp(actor->get_rotation(), target_rotation, weight));

    Vec3 horizontal_velocity = Vec3(character_controller->target_velocity.x, 0.0, character_controller->target_velocity.z);
    horizontal_velocity += movement_direction * float(10.0f * delta);
    if (glm::length(horizontal_velocity) > MAX_AIR_MOVE_SPEED) {
        horizontal_velocity = glm::normalize(horizontal_velocity) * MAX_AIR_MOVE_SPEED;
    }
    character_controller->target_velocity.x = horizontal_velocity.x;
    character_controller->target_velocity.z = horizontal_velocity.z;
}