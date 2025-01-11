#pragma once

#include <components/state_machine.h>
#include <functional>
#include <components/character_controller.h>

namespace State {
    struct Dodge : public State {
        std::vector<std::function<void(std::string)>>::iterator _on_animation_finished;

        void enter() {
            anim_player->play("Roll");
            std::function<void(std::string)> f = std::bind(&Dodge::on_animation_finished, this, std::string(""));
            _on_animation_finished = anim_player->finished.connect(f);
        }

        void update(double delta) override;
        void on_animation_finished(std::string _p_animation);
        void exit() override;
    };
}