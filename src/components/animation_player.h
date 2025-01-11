#pragma once

#include <core/component.h>
#include <core/signal.h>
#include <resources/animation.h>

struct Skeleton;

struct AnimationPlayer : public Component {
    Signal<std::string> finished;

    std::shared_ptr<Skeleton> skeleton;
    AnimationLibrary library;
    std::string current_animation {""};
    float current_time {0.0f};
    bool playing = true;

    Vec3 previous_root_motion_position {};
    Vec3 root_motion_velocity {};

    std::unordered_map<uint32_t, uint32_t> channel_position_index {};
    std::unordered_map<uint32_t, uint32_t> channel_rotation_index {};

    bool updating { false };
    bool interrupted { false };

    void update(double delta) override;

    void play(std::string p_animation_name);

    void initialize() override;

    COMPONENT_FACTORY_H(AnimationPlayer)
};