#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/common.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;

typedef glm::mat3 Mat3;
typedef glm::mat4 Mat4;

typedef glm::quat Quaternion;

struct Transform {
    Vec3 position { 0.0f, 0.0f, 0.0f };
    glm::quat rotation { glm::quat(Vec3(0.0f)) };
    float scale { 1.0f };

    Mat4 get_matrix();
    const Transform operator*(Transform const& rhs);

    Transform() {}
    Transform(Transform const& p_transform);
    Transform(Vec3 p_position, glm::quat p_rotation, float p_scale);
    ~Transform() {}
};

namespace Math {
    template <typename T> T interpolate(T a, T b, float duration, float delta) {
        return a + (b - a) * (1.0f - std::exp(-delta / duration));
    }
}