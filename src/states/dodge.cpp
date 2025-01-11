#include <states/dodge.h>

#include <states/idle.h>

void State::Dodge::on_animation_finished(std::string _p_animation) {
    state_machine->transition_to<Idle>();
}

void State::Dodge::update(double delta) {
    auto character_controller = actor->get_component<CharacterController>();
    Vec3 animated_velocity = actor->get_rotation() * anim_player->root_motion_velocity / float(delta);
    character_controller->target_velocity.x = animated_velocity.x;
    character_controller->target_velocity.z = animated_velocity.z;   
}

void State::Dodge::exit() {
    anim_player->finished.disconnect(_on_animation_finished);
}