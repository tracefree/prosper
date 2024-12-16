#include <components/skeleton.h>
#include <core/node.h>

std::string Skeleton::get_name() {
    return "Skeleton";
}

void Skeleton::set_bone_rotation(uint32_t p_bone_index, glm::quat p_rotation) {
    if (p_bone_index != root_motion_index) {
        bones[p_bone_index]->set_rotation(p_rotation);
    }
}

void Skeleton::set_bone_position(uint32_t p_bone_index, Vec3 p_position) {
    if (p_bone_index != root_motion_index) {
        bones[p_bone_index]->set_position(p_position);
    }
}

void Skeleton::draw(const Mat4& p_transform, DrawContext& p_context) {
    if (joint_matrices_buffer == nullptr) {
        return;
    }
    auto data = (Mat4*)joint_matrices_buffer->info.pMappedData;
    memcpy((void*)&data[0], joint_matrices.data(), joint_matrices.size()*sizeof(Mat4));
}