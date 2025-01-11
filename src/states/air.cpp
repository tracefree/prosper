#include <states/air.h>
#include <components/character_controller.h>
#include <states/idle.h>
#include <physics.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

void State::Air::update(double delta) {
    auto character_controller = actor->get_component<CharacterController>();
    character_controller->target_velocity.y -= 9.81f * float(delta);

    const bool grounded = (character_controller->character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround);
    if (grounded) {
        state_machine->transition_to<Idle>();
    }
}