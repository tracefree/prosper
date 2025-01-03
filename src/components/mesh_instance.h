#pragma once
#include <core/component.h>
#include <core/resource.h>
#include <resources/mesh.h>

struct DrawContect;

struct MeshInstance : public Component {
    std::shared_ptr<Resource<Mesh>> mesh;

    void draw(const Mat4& p_transform, DrawContext& p_context) const override;

    MeshInstance() {}
};