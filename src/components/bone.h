#pragma once

#include <core/component.h>
#include <components/skeleton.h>
#include <memory>

struct Bone : public Component {
    Mat4 inverse_bind_matrix;
    int index;
    std::shared_ptr<Skeleton> skeleton;
    void update(double _delta) override;
    static std::string get_name();
};