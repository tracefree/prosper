#pragma once
#include <core/component.h>

struct PointLight : public Component {
    Vec3 color;
    float intensity;

    void update(double _delta) override;

    COMPONENT_FACTORY_H(PointLight)
};