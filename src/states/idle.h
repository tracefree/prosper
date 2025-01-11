#pragma once

#include <components/state_machine.h>
#include <input.h>

namespace State {
    struct Move;
    struct Idle : public State {
        void enter() {
            anim_player->play("Idle-loop");
        }

        void update(double delta) override;
    };
}