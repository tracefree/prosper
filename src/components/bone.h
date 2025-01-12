#pragma once

#include <core/component.h>
#include <util.h>

struct Skeleton;

struct Bone : public Component {
    Mat4 inverse_bind_matrix;
    int index;
    Ref<Skeleton> skeleton;
    
    void update(double _delta) override;
};