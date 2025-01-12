#pragma once

#include <core/component.h>
#include <math.h>
#include <util.h>

struct DrawContext;
struct AllocatedBuffer;

struct Skeleton : public Component {
    std::vector<Mat4> joint_matrices;
    AllocatedBuffer* joint_matrices_buffer;
    std::vector<Ref<Node>> bones;
    int root_motion_index {-1};
    
    void set_bone_rotation(uint32_t p_bone_index, glm::quat p_rotation);
    void set_bone_position(uint32_t p_bone_index, Vec3 p_position);
    void draw(const Mat4& p_transform, DrawContext& p_context) const override;
};