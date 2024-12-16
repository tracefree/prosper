#pragma once
#include <rendering/types.h>
#include <core/component.h>

struct RigidBody : public Component {
    static std::string get_name();
};