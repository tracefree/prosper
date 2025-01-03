#pragma once

#include <util.h>
#include <resources/animation.h>

#include <filesystem>
#include <fastgltf/core.hpp>

#include <components/model_data.h>


std::optional<std::shared_ptr<Node>> load_gltf_scene(Renderer* p_renderer, std::filesystem::path p_file_path);

void import_gltf_scene(Renderer* p_renderer, std::filesystem::path p_file_path);

std::optional<AllocatedImage> load_image(Renderer* p_renderer, std::filesystem::path p_file_path, Format p_format = Format::eR8G8B8A8Unorm);

std::optional<AllocatedImage> load_image(Renderer* p_renderer, fastgltf::Asset& p_asset, fastgltf::Image& p_image, Format p_format = Format::eR8G8B8A8Unorm);