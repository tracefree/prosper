#pragma once

#include <core/component.h>
#include <memory>
#include <rendering/types.h>

struct Skeleton : public Component {
    std::vector<Mat4> joint_matrices;
    AllocatedBuffer* joint_matrices_buffer;
    std::vector<std::shared_ptr<Node>> bones;
    int root_motion_index {-1};
    
    void set_bone_rotation(uint32_t p_bone_index, glm::quat p_rotation);
    void set_bone_position(uint32_t p_bone_index, Vec3 p_position);
    void draw(const Mat4& p_transform, DrawContext& p_context) override;
    static std::string get_name();
};