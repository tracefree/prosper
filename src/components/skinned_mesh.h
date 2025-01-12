#pragma once

#include <core/component.h>
#include <core/resource.h>
#include <memory>
#include <rendering/types.h>
#include <resources/mesh.h>
#include <util.h>

struct SkinnedMesh : public Component {
    Ref<Resource<Mesh>> mesh;

    void draw(const Mat4& p_transform, DrawContext& p_context) const override;
    void cleanup() override;
};