#include <components/bone.h>

#include <core/node.h>


void Bone::update(double _delta) {
    if (skeleton == nullptr) {
        return;
    }
    skeleton->joint_matrices[index] =
        glm::inverse(skeleton->node->get_global_transform().get_matrix()) * 
        node->get_global_transform().get_matrix() * 
        inverse_bind_matrix;
}

std::string Bone::get_name() {
    return "Bone";
}