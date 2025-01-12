#pragma once
#include <core/component.h>
#include <core/resource.h>
#include <resources/mesh.h>
#include <util.h>

struct DrawContect;

struct MeshInstance : public Component {
    Ref<Resource<Mesh>> mesh;

    void draw(const Mat4& p_transform, DrawContext& p_context) const override;

    MeshInstance() {}
};