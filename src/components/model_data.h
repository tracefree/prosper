#pragma once

#include <core/component.h>
#include <core/resource.h>
#include <components/animation_player.h>
#include <resources/mesh.h>
#include <resources/texture.h>
#include <rendering/types.h>
#include <rendering/descriptors.h>

class Renderer;
struct Skeleton;

struct ModelData : public Component {
    std::unordered_map<std::string, Ref<Resource<Mesh>>> meshes;
    std::vector<Ref<Resource<Texture>>> textures;
    std::unordered_map<std::string, Ref<MaterialInstance>> materials;

    std::vector<Sampler> samplers;
    DescriptorAllocatorGrowable descriptor_pool;

    AllocatedBuffer material_data_buffer;
    Renderer* renderer;

    std::string model_path;
    Ref<Skeleton> skeleton;
    AnimationLibrary animation_library {};
    bool skinned { false };
    int root_motion_index { -1 };

    void initialize() override;
    virtual void cleanup() override;

    COMPONENT_FACTORY_H(ModelData)
};