#pragma once
#include <core/component.h>
#include <rendering/types.h>

struct PointLight : public Component {
    Vec3 color;
    float intensity;

    void update(double _delta) override;
    static std::string get_name();
};