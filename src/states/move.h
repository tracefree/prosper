#pragma once

#include <components/state_machine.h>
#include <input.h>

namespace State {
    struct Move : public State {
        void enter() {
            anim_player->play("Run-loop");
        }

        void update(double delta) override;
        void exit() override;
    };
}