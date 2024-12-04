#pragma once

#include "vk_types.h"
#include "vk_descriptors.h"

#include <memory>
#include <unordered_map>
#include <filesystem>
#include <fastgltf/core.hpp>


class Renderer;

//std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_gltf_meshes(Renderer* p_renderer, std::filesystem::path p_file_path);

std::optional<AllocatedImage> load_image(Renderer* p_renderer, std::filesystem::path p_file_path);

struct LoadedGLTF : public IRenderable {
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> textures;
    std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> materials;

    std::vector<std::shared_ptr<Node>> root_nodes;
    std::vector<Sampler> samplers;
    DescriptorAllocatorGrowable descriptor_pool;

    AllocatedBuffer material_data_buffer;
    Renderer* renderer;

    virtual void draw(const Mat4& p_transform, DrawContext& p_context);
    ~LoadedGLTF() { clear_all(); }

private:
    void clear_all();
};

std::optional<std::shared_ptr<LoadedGLTF>> load_gltf_scene(Renderer* p_renderer, std::filesystem::path p_file_path);
std::optional<AllocatedImage> load_image(Renderer* p_renderer, fastgltf::Asset& p_asset, fastgltf::Image& p_image);