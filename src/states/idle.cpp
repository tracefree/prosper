#include <states/idle.h>

#include <states/move.h>
#include <states/air.h>
#include <components/character_controller.h>

void State::Idle::update(double delta) {
    const Vec2 movement_input = Input::get_movement_input();
    if (glm::length(movement_input) > 0.0f) {
        state_machine->transition_to<Move>();
        return;
    }

    const auto character_controller = actor->get_component<CharacterController>();
    if (Input::is_pressed(SDL_SCANCODE_SPACE)) {
        character_controller->target_velocity.y = 4.0f;
        state_machine->transition_to<Air>();
        return;
    }

    character_controller->target_velocity = Vec3(0.0f, -9.81f, 0.0f);
}