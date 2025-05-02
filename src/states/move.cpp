#include <states/move.h>
#include <states/idle.h>
#include <components/character_controller.h>
#include <physics.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <core/scene_graph.h>
#include <components/camera.h>
#include <states/air.h>
#include <states/dodge.h>

extern SceneGraph scene;

void State::Move::update(double delta) {
    auto character_controller = actor->get_component<CharacterController>();

    const bool grounded = (character_controller->character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround);
    if (!grounded) {
        state_machine->transition_to<Air>();
        return;
    }

    if(Input::is_pressed(SDL_SCANCODE_LALT)) {
        // TODO: Find out why dodge rolling segfaults in some builds.
        // state_machine->transition_to<Dodge>();
        return;
    }

    if (anim_player->current_animation != "Run-loop" && !Input::is_pressed(SDL_SCANCODE_LSHIFT)) {
        anim_player->play("Run-loop");
    } else if (anim_player->current_animation != "Sprint-loop" && Input::is_pressed(SDL_SCANCODE_LSHIFT)) {
        anim_player->play("Sprint-loop");
    }
    character_controller->target_velocity.y = (Input::is_pressed(SDL_SCANCODE_SPACE)) ? state_machine->jump_speed : 0.0f;
    

    const Vec2 movement_input = Input::get_movement_input();
    if (glm::length2(movement_input) == 0.0f) {
        state_machine->transition_to<Idle>();
        return;
    }

    const Vec3 movement_direction = Mat3(scene.camera->get_component<Camera>()->get_horizontal_rotation_matrix()) * Vec3(movement_input.x, 0.0f, -movement_input.y);
    glm::quat target_rotation = glm::quatLookAt(-movement_direction, Vec3(0.0f, 1.0f, 0.0f));
    float weight = (1.0f - std::exp(-(float(delta)/0.025f)));
    actor->set_rotation(glm::slerp(actor->get_rotation(), target_rotation, weight));

    Vec3 animated_velocity = actor->get_rotation() * anim_player->root_motion_velocity / float(delta);
    character_controller->target_velocity.x = animated_velocity.x;
    character_controller->target_velocity.z = animated_velocity.z;   
}

void State::Move::exit() {
    
}