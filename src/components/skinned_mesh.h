#pragma once

#include <core/component.h>
#include <memory>
#include <rendering/types.h>

struct SkinnedMesh : public Component {
    std::shared_ptr<MeshAsset> mesh;

    void draw(const Mat4& p_transform, DrawContext& p_context) override;
    static std::string get_name();
};