#include <components/animation_player.h>
#include <core/node.h>

std::string AnimationPlayer::get_name() {
    return "AnimationPlayer";
}

void AnimationPlayer::update(double delta) {
    if (!playing || library.animations.find(current_animation) == library.animations.end()) {
        return;
    }

    auto animation = library.animations[current_animation];
    
    current_time = current_time + float(delta);
    bool looped = current_time > animation.length;
    if (looped) {
        current_time -= animation.length;
    }

    for (auto kv : animation.channels) {
        uint32_t bone_index = kv.first;
        AnimationChannel channel = kv.second;
        if (channel.position_keyframes.size() > 2) {
            uint32_t current_index = channel_position_index[kv.first];
            uint32_t next_index = (current_index + 1) < (channel.position_keyframes.size() - 1) ?
                (current_index + 1) : 0;
            auto current_keyframe = channel.position_keyframes[current_index];
            auto next_keyframe    = channel.position_keyframes[next_index];
            
            if (
                (current_time >= next_keyframe.time) && 
                (current_keyframe.time < next_keyframe.time || current_time < current_keyframe.time)
            ) {
                current_index = next_index;
                next_index = (current_index + 1) < (channel.position_keyframes.size() - 1) ?
                    (current_index + 1) : 0;
                current_keyframe = channel.position_keyframes[current_index];
                next_keyframe    = channel.position_keyframes[next_index];
                channel_position_index[kv.first] = current_index;
            }

            float time_difference = next_keyframe.time - current_keyframe.time;
            if (time_difference < 0) {
                time_difference += animation.length;
            }
            float elapsed_time = current_time - current_keyframe.time;
            if (elapsed_time < 0) {
                elapsed_time += animation.length;
            }
            float weight = elapsed_time / time_difference;
            
            Vec3 blended_position = current_keyframe.position * (1.0f - weight) + next_keyframe.position * weight;
            skeleton->set_bone_position(bone_index, blended_position);
        } else if (channel.position_keyframes.size() == 2) {
            auto current_keyframe = channel.position_keyframes[0];
            auto next_keyframe    = channel.position_keyframes[1];

            float time_difference = next_keyframe.time - current_keyframe.time;
            if (time_difference < 0) {
                time_difference += animation.length;
            }

            float elapsed_time = current_time - current_keyframe.time;
            if (elapsed_time < 0) {
                elapsed_time += animation.length;
            }
            float weight = elapsed_time / time_difference;

            Vec3 blended_position = current_keyframe.position * (1.0f - weight) + next_keyframe.position * weight;
            skeleton->set_bone_position(bone_index, blended_position);
        } else if (channel.position_keyframes.size() == 1) {
            skeleton->set_bone_position(bone_index, channel.position_keyframes[0].position);
        }
        
        if (channel.rotation_keyframes.size() > 2) {
            uint32_t current_index = channel_rotation_index[kv.first];
            uint32_t next_index = (current_index + 1) < (channel.rotation_keyframes.size() - 1) ?
                (current_index + 1) : 0;
            auto current_keyframe = channel.rotation_keyframes[current_index];
            auto next_keyframe    = channel.rotation_keyframes[next_index];
            
            if (
                (current_time >= next_keyframe.time) && 
                (current_keyframe.time < next_keyframe.time || current_time < current_keyframe.time)
            ) {
                current_index = next_index;
                next_index = (current_index + 1) < (channel.rotation_keyframes.size() - 1) ?
                    (current_index + 1) : 0;
                current_keyframe = channel.rotation_keyframes[current_index];
                next_keyframe    = channel.rotation_keyframes[next_index];
                channel_rotation_index[kv.first] = current_index;
            }

            float time_difference = next_keyframe.time - current_keyframe.time;
            if (time_difference < 0) {
                time_difference += animation.length;
            }
            float elapsed_time = current_time - current_keyframe.time;
            if (elapsed_time < 0) {
                elapsed_time += animation.length;
            }
            float weight = elapsed_time / time_difference;
            
            Quaternion blended_rotation = looped ? next_keyframe.rotation : glm::slerp(current_keyframe.rotation, next_keyframe.rotation, weight);
            skeleton->set_bone_rotation(bone_index, blended_rotation);
        } else if (channel.rotation_keyframes.size() == 2) {
            auto current_keyframe = channel.rotation_keyframes[0];
            auto next_keyframe    = channel.rotation_keyframes[1];

            float time_difference = next_keyframe.time - current_keyframe.time;
            if (time_difference < 0) {
                time_difference += animation.length;
            }
            float elapsed_time = current_time - current_keyframe.time;
            if (elapsed_time < 0) {
                elapsed_time += animation.length;
            }
            float weight = elapsed_time / time_difference;
            
            Quaternion blended_rotation = looped ? next_keyframe.rotation : glm::slerp(current_keyframe.rotation, next_keyframe.rotation, weight);
            skeleton->set_bone_rotation(bone_index, blended_rotation);
        } else if (channel.position_keyframes.size() == 1) {
            skeleton->set_bone_rotation(bone_index, channel.rotation_keyframes[0].rotation);
        }
    }
    
    skeleton->node->refresh_transform(Transform()); // TODO: Why?
}

void AnimationPlayer::play(std::string p_animation_name) {
    channel_position_index.clear();
    channel_rotation_index.clear();

    current_animation = p_animation_name;
    if (library.animations.find(current_animation) == library.animations.end()) {
        playing = false;
        return;
    }
    auto animation = library.animations[current_animation];

    for (auto& kv : animation.channels) {
        channel_position_index[kv.first] = 0;
        channel_rotation_index[kv.first] = 0;
    }

    current_time = 0.0f;
    playing = true;
}