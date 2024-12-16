#include <math.h>


Transform::Transform(Vec3 p_position, glm::quat p_rotation, float p_scale) {
    position = p_position;
    rotation = p_rotation;
    scale    = p_scale;
}

Transform::Transform(Transform const& p_transform) {
    position = p_transform.position;
    rotation = p_transform.rotation;
    scale    = p_transform.scale;
}

Mat4 Transform::get_matrix() {
    return glm::translate(Mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(Mat4(1.0f), Vec3(scale));
}

const Transform Transform::operator*(Transform const& rhs) {
    return Transform {
        position + rotation * (scale * rhs.position),
        rotation * rhs.rotation,
        scale * rhs.scale,
    };
}

