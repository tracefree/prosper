#include <components/state_machine.h>
#include <yaml.h>

#include <states/idle.h>

void StateMachine::initialize() {
    transition_to<State::Idle>();
}

COMPONENT_FACTORY_IMPL(StateMachine, state_machine) {

}