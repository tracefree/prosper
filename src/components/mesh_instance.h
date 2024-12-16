#pragma once
#include <core/component.h>
#include <rendering/types.h>
#include <memory>


struct MeshInstance : public Component {
    std::shared_ptr<MeshAsset> mesh;

    void draw(const Mat4& p_transform, DrawContext& p_context) override;
    static std::string get_name();

    MeshInstance(std::shared_ptr<MeshAsset> p_mesh);
};