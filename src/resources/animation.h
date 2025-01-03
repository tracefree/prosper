#pragma once

#include <math.h>
#include <unordered_map>

struct Keyframe {
    float time;
};

struct KeyframePosition : public Keyframe {
    Vec3 position;
};

struct KeyframeRotation : public Keyframe {
    Quaternion rotation;
};

struct AnimationChannel {
    // TODO: Generalized node path
    std::vector<KeyframePosition> position_keyframes;
    std::vector<KeyframeRotation> rotation_keyframes;
};

struct Animation {
    std::string name;
    float length;
    std::unordered_map<uint32_t, AnimationChannel> channels;
};

struct AnimationLibrary {
    std::unordered_map<std::string, Animation> animations;
    std::vector<std::string> get_animation_list() const;
    std::vector<const char*> get_animation_list_cstr() const;
};