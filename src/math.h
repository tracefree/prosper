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

/*
class Vec3 {
    glm::vec3 vector {};
public:
    float x;
    float y;
    float z;
    Vec3();
    Vec3(float p_x, float p_y, float p_z);
};

class Vec4 {
    glm::vec4 vector {};
public:
    float x;
    float y;
    float z;
    float w;

    Vec4();
    Vec4(float x, float y, float z, float w);
    Vec4(Vec3 xyz);
};

class Mat4 {
    glm::mat4 matrix;
public:
    Mat4();
};

class Quaternion {
    
};
*/