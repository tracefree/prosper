#pragma once

#include <util.h>
#include <core/component.h>
#include <core/resource.h>
#include <rendering/types.h>
#include <rendering/descriptors.h>
#include <resources/animation.h>
#include <resources/mesh.h>

#include <memory>
#include <unordered_map>
#include <filesystem>
#include <fastgltf/core.hpp>


class Renderer;

struct LoadedGLTF : public Component {
    std::unordered_map<std::string, std::string> meshes;
    std::unordered_map<std::string, AllocatedImage> textures;
    std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> materials;

    std::vector<Sampler> samplers;
    DescriptorAllocatorGrowable descriptor_pool;

    AllocatedBuffer material_data_buffer;
    Renderer* renderer;

    void load(std::string p_path);
    static std::string get_name();
    virtual void cleanup() override;
};

std::optional<std::shared_ptr<Node>> load_gltf_scene(Renderer* p_renderer, std::filesystem::path p_file_path);

void import_gltf_scene(Renderer* p_renderer, std::filesystem::path p_file_path);

std::optional<AllocatedImage> load_image(Renderer* p_renderer, std::filesystem::path p_file_path, Format p_format = Format::eR8G8B8A8Unorm);

std::optional<AllocatedImage> load_image(Renderer* p_renderer, fastgltf::Asset& p_asset, fastgltf::Image& p_image, Format p_format = Format::eR8G8B8A8Unorm);

std::string filesystem_path(std::string p_path);

std::shared_ptr<Node> load_scene(std::string p_path);

std::tuple<std::vector<Vec3>, std::vector<uint>> load_triangles(std::filesystem::path p_file_path);