#pragma once

#include <components/state_machine.h>

namespace State {
    struct Air : public State {
        void enter() {
            anim_player->play("Air-loop");
        }

        void update(double delta) override;
    };
}