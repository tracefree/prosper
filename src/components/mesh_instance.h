#pragma once
#include <core/component.h>
#include <core/resource.h>
#include <rendering/types.h>
#include <resources/mesh.h>
#include <memory>


struct MeshInstance : public Component {
    std::shared_ptr<Resource<Mesh>> mesh;

    void draw(const Mat4& p_transform, DrawContext& p_context) override;
    static std::string get_name();

    MeshInstance() {}
};