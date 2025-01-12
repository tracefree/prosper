#pragma once

#include <components/animation_player.h>
#include <core/node.h>
#include <core/component.h>
#include <core/signal.h>

#include <unordered_map>
#include <string>
#include <print>
#include <type_traits>
#include <memory>


struct StateMachine;

namespace State {
    struct State {
        StateMachine* state_machine;
        Node* actor;
        Ref<AnimationPlayer> anim_player;

        template<typename... Ts>
        void enter(Ts... p_config);
        
        virtual void update(double delta) {}
        virtual void exit() {}

        State() {}
        State(Node* p_actor) {
            actor = p_actor;
        }
    };
};

struct StateMachine : public Component {
    std::unordered_map<const std::type_info*, std::shared_ptr<State::State>> states;
    std::shared_ptr<State::State> state { nullptr };

    Signal<std::string> transitioned;

    template<typename T, typename... Ts>
    std::enable_if_t<std::is_base_of_v<State::State, T>, void>
    transition_to(Ts... args) {
        if (state != nullptr) {
            state->exit();
        }
        if (!states.contains(&typeid(T))) {
            register_state<T>();
        }
        state = states[&typeid(T)];
        (static_pointer_cast<T>(state))->enter(args...);
        transitioned.emit("state");
    }

    template<typename T>
    std::enable_if_t<std::is_base_of_v<State::State, T>, void>
    register_state() {
        states[&typeid(T)] = std::make_shared<T>();
        states[&typeid(T)]->state_machine = this;
        states[&typeid(T)]->actor = node;
        states[&typeid(T)]->anim_player = node->get_component<AnimationPlayer>();
    }

    void update(double delta) override {
        if (state == nullptr) return;
        state->update(delta);
    }

    void initialize() override;

    COMPONENT_FACTORY_H(StateMachine);
};