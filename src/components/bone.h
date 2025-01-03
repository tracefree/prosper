#pragma once

#include <core/component.h>

struct Skeleton;

struct Bone : public Component {
    Mat4 inverse_bind_matrix;
    int index;
    std::shared_ptr<Skeleton> skeleton;
    
    void update(double _delta) override;
};