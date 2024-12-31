#include <loader.h>

#include <rendering/renderer.h>
#include <util.h>
#include <resources/animation.h>
#include <components/animation_player.h>
#include <components/bone.h>
#include <components/skeleton.h>
#include <components/mesh_instance.h>
#include <components/point_light.h>
#include <components/skinned_mesh.h>
#include <core/node.h>
#include <core/resource_manager.h>
#include <resources/mesh.h>
#include <resources/texture.h>

#include <stb_image.h>
#include <iostream>
#include <fstream>
#include <span>

#include <variant>
#include <print>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <ktxvulkan.h>

#include <yaml-cpp/yaml.h>

using namespace vk;

// TODO: Break down this whole file

extern Renderer gRenderer; // TODO remove
extern std::shared_ptr<Bone> target_bone;
extern std::shared_ptr<Skeleton> target_skeleton;
extern std::shared_ptr<AnimationPlayer> animation_player;

Filter extract_filter(fastgltf::Filter filter) {
    switch(filter) {
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::NearestMipMapLinear:
            return Filter::eNearest;
        
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return Filter::eLinear;
    }
}

SamplerAddressMode extract_address_mode(fastgltf::Wrap wrap) {
    switch (wrap) {
        case fastgltf::Wrap::ClampToEdge:
            return SamplerAddressMode::eClampToEdge;
        case fastgltf::Wrap::MirroredRepeat:
            return SamplerAddressMode::eMirroredRepeat;
        case fastgltf::Wrap::Repeat:
            return SamplerAddressMode::eRepeat;
        deftault:
            return SamplerAddressMode::eRepeat;
    }
}

SamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
{
    switch (filter) {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return SamplerMipmapMode::eNearest;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
    default:
        return SamplerMipmapMode::eLinear;
    }
}

std::optional<AllocatedImage> load_image(Renderer* p_renderer, std::filesystem::path p_file_path, Format p_format) {
    AllocatedImage new_image {};
    if (p_file_path.extension() == ".ktx2") {
        ktxTexture2* k_texture;
        auto result = ktxTexture2_CreateFromNamedFile(p_file_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &k_texture);
        if (result != KTX_SUCCESS) {
            print("Could not create KTX texture!");
        }

        auto color_model = ktxTexture2_GetColorModel_e(k_texture);
        ktx_transcode_fmt_e texture_format;
        PhysicalDeviceFeatures features = p_renderer->physical_device.getFeatures();
        if (color_model != KHR_DF_MODEL_RGBSDA) {
            if (color_model == KHR_DF_MODEL_UASTC && features.textureCompressionASTC_LDR) {
                texture_format = KTX_TTF_ASTC_4x4_RGBA;
            } else if (color_model == KHR_DF_MODEL_ETC1S && features.textureCompressionETC2) {
                texture_format = KTX_TTF_ETC;
            } else if (features.textureCompressionASTC_LDR) {
                texture_format = KTX_TTF_ASTC_4x4_RGBA;
            } else if (features.textureCompressionETC2) {
                texture_format = KTX_TTF_ETC2_RGBA;
            } else if (features.textureCompressionBC) {
                texture_format = KTX_TTF_BC3_RGBA;
            } else {
                print("Could not transcode texture!");
                return {};
            }
            result = ktxTexture2_TranscodeBasis(k_texture, KTX_TTF_BC7_RGBA, 0);
            if (result != KTX_SUCCESS) {
                print("Could not transcode texture!");
            }
        }
        
        ktxVulkanTexture texture;
        result = ktxTexture2_VkUploadEx(k_texture, &p_renderer->ktx_device_info, &texture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if (result != KTX_SUCCESS) {
            print("Could not upload KTX texture!");
        }
        new_image.ktx_texture = texture;
        new_image.image = texture.image;

        ImageViewCreateInfo view_info {
            .image      = texture.image,
            .viewType   = static_cast<ImageViewType>(texture.viewType),
            .format     = static_cast<Format>(texture.imageFormat),
            .subresourceRange = ImageSubresourceRange {
                .aspectMask     = ImageAspectFlagBits::eColor,
                .baseMipLevel   = 0,
                .levelCount     = texture.levelCount,
                .baseArrayLayer = 0,
                .layerCount     = texture.layerCount,
            }
        };
        Result view_result;
        std::tie(view_result, new_image.image_view) = p_renderer->device.createImageView(view_info);
        ktxTexture2_Destroy(k_texture);
        return new_image;
    }
    int w, h, nrChannels;
    unsigned char* data = stbi_load(p_file_path.c_str(), &w, &h, &nrChannels, 4);
    new_image = p_renderer->create_image(data, Extent3D {uint32_t(w), uint32_t(h), 1}, p_format, ImageUsageFlagBits::eSampled, true);
    return new_image;
}

std::optional<AllocatedImage> load_image(Renderer* p_renderer, fastgltf::Asset& p_asset, fastgltf::Image& p_image, Format p_format) {
    // TODO: Detect normal map and choose format accordingly
    AllocatedImage new_image {};
    int width, height, number_channels;
    std::visit(
        fastgltf::visitor {
            [](auto& argument) {},
            [&](fastgltf::sources::URI& file_path) {
                assert(file_path.fileByteOffset == 0);
                assert(file_path.uri.isLocalPath());
                // ! TODO: find correct folder for texture assets relative to gltf file instead of hardcoding
                auto full_path = std::string("assets/models/Sponza/glTF/") + file_path.uri.c_str();
                
                unsigned char* data = stbi_load(full_path.c_str(), &width, &height, &number_channels, 4);
                
                if (data) {
                    Extent3D image_size {
                        .width  = uint32_t(width),
                        .height = uint32_t(height),
                        .depth  = 1,
                    };
                    
                    new_image = p_renderer->create_image(data, image_size, p_format, ImageUsageFlagBits::eSampled, true);
                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::Array& array) {
                unsigned char* data = stbi_load_from_memory((stbi_uc*)array.bytes.data(), static_cast<int>(array.bytes.size()), &width, &height, &number_channels, 4);
                if (data) {
                    Extent3D image_size {
                        .width  = uint32_t(width),
                        .height = uint32_t(height),
                        .depth  = 1,
                    };
                    new_image = p_renderer->create_image(data, image_size, p_format, ImageUsageFlagBits::eSampled, true);
                    stbi_image_free(data);
                }
            },
            [&](fastgltf::sources::BufferView& view) {
                auto& buffer_view = p_asset.bufferViews[view.bufferViewIndex];
                auto& buffer = p_asset.buffers[buffer_view.bufferIndex];
                std::visit(
                    fastgltf::visitor {
                        [](auto& argument) {},
                        [&](fastgltf::sources::Array& array) {
                            unsigned char* data = stbi_load_from_memory((stbi_uc*)array.bytes.data() + buffer_view.byteOffset, static_cast<int>(buffer_view.byteLength), &width, &height, &number_channels, 4);
                            if (data) {
                                Extent3D image_size {
                                    .width  = uint32_t(width),
                                    .height = uint32_t(height),
                                    .depth  = 1,
                                };
                                new_image = p_renderer->create_image(data, image_size, p_format, ImageUsageFlagBits::eSampled, true);
                                stbi_image_free(data);
                            }
                        }
                    }, buffer.data
                );
            },
        }, p_image.data
    );

    if (new_image.image == VK_NULL_HANDLE) {
        return {};
    } else {
        return new_image;
    }
}


// --- LoadedGLTF ---

void LoadedGLTF::cleanup() {
    Device device = renderer->device;
    descriptor_pool.destroy_pools(device);
    renderer->destroy_buffer(material_data_buffer);

    for (auto& [key, value] : meshes) {
        value->unreference();
        //value.unreference();
        /*
        renderer->destroy_buffer(value->mesh_buffers.index_buffer);
        renderer->destroy_buffer(value->mesh_buffers.vertex_buffer);
        renderer->destroy_buffer(value->mesh_buffers.skinned_vertex_buffer);
        if (value->mesh_buffers.skinning_data_buffer.buffer != VK_NULL_HANDLE) {
            renderer->destroy_buffer(value->mesh_buffers.skinning_data_buffer);
        }
        if (value->mesh_buffers.joint_matrices_buffer.buffer != VK_NULL_HANDLE) {
            renderer->destroy_buffer(value->mesh_buffers.joint_matrices_buffer);
        }
        */
    }

    for (auto& texture : textures) {
        (*texture).unreference();
    }

    for (auto& sampler : samplers) {
        device.destroySampler(sampler);
    }
}

void LoadedGLTF::load(std::string p_path) {
    print("Loading imported GLTF: %s", p_path.c_str());
    std::fstream file(p_path, std::fstream::in | std::fstream::binary);
    
    auto mesh_guid = std::format("{}::/meshes/{}", p_path.c_str(), "x");
    meshes["x"] = ResourceManager::get<Mesh>(mesh_guid.c_str());
    auto& mesh = *meshes["x"];
    
    materials["default"] = std::make_shared<MaterialInstance>(gRenderer.default_material);

    // Samplers
    std::string line;
    std::getline(file, line);
    assert(line == "Samplers");
    std::getline(file, line);
    uint32_t sampler_count = stoi(line);

    std::vector<SamplerCreateInfo> sampler_infos;
    sampler_infos.resize(sampler_count);
    file.read(reinterpret_cast<char*>(sampler_infos.data()), sampler_count * sizeof(SamplerCreateInfo));
    for (auto sampler_info : sampler_infos) {
        auto[result, new_sampler] = gRenderer.device.createSampler(sampler_info);
        samplers.push_back(new_sampler);
    }

    // Textures
    std::vector<std::optional<AllocatedImage>> temp_textures;
    std::vector<std::string> texture_paths;
    std::getline(file, line);
    assert(line == "Textures");
    std::getline(file, line);
    uint32_t texture_count = std::stoi(line);
    temp_textures.resize(texture_count);
    for (uint32_t texture_index = 0; texture_index < texture_count; texture_index++) {
        std::getline(file, line);
        texture_paths.push_back(line);
    }

    // Material data
    std::getline(file, line);
    assert(line == "Materials");
    std::getline(file, line);
    uint32_t material_count = std::stoi(line);

    PhysicalDeviceProperties physical_device_properties = gRenderer.physical_device.getProperties();
    uint32_t min_offset_multiplier = ceilf(float(sizeof(MaterialMetallicRoughness::MaterialResources)) / float(physical_device_properties.limits.minUniformBufferOffsetAlignment));

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        { DescriptorType::eCombinedImageSampler, 3},
        { DescriptorType::eUniformBuffer, 3},
        { DescriptorType::eStorageBuffer, 1}
    };

    descriptor_pool.init_pool(gRenderer.device, materials.size(), sizes);

    material_data_buffer = gRenderer.create_buffer(
        sizeof(MaterialMetallicRoughness::MaterialConstants) * material_count,
        BufferUsageFlagBits::eUniformBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    MaterialMetallicRoughness::MaterialConstants* scene_material_constants = (MaterialMetallicRoughness::MaterialConstants*) material_data_buffer.info.pMappedData;

    for (uint32_t material_index = 0; material_index < material_count; material_index++) {
        MaterialMetallicRoughness::MaterialConstants constants {
            .albedo_factors = Vec4(1.0f),
            .metal_roughness_factors = Vec4(0.0f, 1.0f, 0.0f, 0.0f)
        };

        std::getline(file, line);
        constants.albedo_factors.r = std::stof(line);
        std::getline(file, line);
        constants.albedo_factors.g = std::stof(line);
        std::getline(file, line);
        constants.albedo_factors.b = std::stof(line);
        std::getline(file, line);
        constants.albedo_factors.a = std::stof(line);
        std::getline(file, line);
        constants.metal_roughness_factors.r = std::stof(line);
        std::getline(file, line);
        constants.metal_roughness_factors.g = std::stof(line);
        
        scene_material_constants[material_index] = constants;
        
        MaterialMetallicRoughness::MaterialResources material_resources {
            .albedo_texture = ResourceManager::get<Texture>("::image_white"),
            .albedo_sampler = gRenderer.sampler_default_nearest,
            .normal_texture = ResourceManager::get<Texture>("::image_default_normal"),
            .normal_sampler = gRenderer.sampler_default_nearest,
            .metal_roughness_texture = ResourceManager::get<Texture>("::image_white"),
            .metal_roughness_sampler = gRenderer.sampler_default_linear,
            .data_buffer = material_data_buffer.buffer,
            .data_buffer_offset = material_index * (uint32_t)sizeof(MaterialMetallicRoughness::MaterialConstants),
        };

        // Albedo
        std::getline(file, line);
        int albedo_image_index = std::stoi(line);
        if (albedo_image_index > -1) {
            auto texture_guid = texture_paths[albedo_image_index].c_str();
            material_resources.albedo_texture = ResourceManager::get<Texture>(texture_guid);
            auto &tex = *material_resources.albedo_texture;
            if (!tex.loaded()) {
                ResourceManager::load<Texture>(texture_guid, vk::Format::eR8G8B8A8Srgb);
                tex.set_load_status(LoadStatus::LOADED);
            } else {
                std::println("Reusing texture: {}", texture_guid);
            }
            tex.reference();
            textures.push_back(material_resources.albedo_texture);
        }
        std::getline(file, line);
        int albedo_sampler_index = std::stoi(line);
        if (albedo_sampler_index > -1) {
            material_resources.albedo_sampler = samplers[albedo_sampler_index];
        }

        // Normal
        std::getline(file, line);
        int normal_image_index = std::stoi(line);
        if (normal_image_index > -1) {
            auto texture_guid = texture_paths[normal_image_index].c_str();
            material_resources.normal_texture = ResourceManager::get<Texture>(texture_guid);
            if (!material_resources.normal_texture->loaded()) {
                ResourceManager::load<Texture>(texture_guid, vk::Format::eR8G8B8A8Unorm);
            } else {
                std::println("Reusing texture: {}", texture_guid);
            }
            material_resources.normal_texture->reference();
            textures.push_back(material_resources.normal_texture);
        }
        std::getline(file, line);
        int normal_sampler_index = std::stoi(line);
        if (normal_sampler_index > -1) {
            material_resources.normal_sampler = samplers[normal_sampler_index];
        }

        // Metal/Roughness
        std::getline(file, line);
        int metal_roughness_image_index = std::stoi(line);
        if (metal_roughness_image_index > -1) {
            auto texture_guid = texture_paths[metal_roughness_image_index].c_str();
            material_resources.metal_roughness_texture = ResourceManager::get<Texture>(texture_guid);
            if (!material_resources.metal_roughness_texture->loaded()) {
                ResourceManager::load<Texture>(texture_guid, vk::Format::eR8G8B8A8Unorm);
            } else {
                std::println("Reusing texture: {}", texture_guid);
            }
            material_resources.metal_roughness_texture->reference();
            textures.push_back(material_resources.metal_roughness_texture);
        }
        std::getline(file, line);
        int metal_roughness_sampler_index = std::stoi(line);
        if (metal_roughness_sampler_index > -1) {
            material_resources.metal_roughness_sampler = samplers[metal_roughness_sampler_index];
        }

        auto material = gRenderer.metal_roughness_material.write_material(gRenderer.device, MaterialPass::MainColor, material_resources, descriptor_pool);
        materials[std::to_string(material_index)] = std::make_shared<MaterialInstance>(material);
    }
    
    // Animations
    std::getline(file, line);
    assert(line == "Animations");
    std::getline(file, line);
    int animation_count = std::stoi(line);
    AnimationLibrary animation_library {};
    for (uint32_t animation_index = 0; animation_index < animation_count; animation_index++) {
        Animation animation {};
        std::getline(file, line);
        std::string animation_name = line;
        
        std::getline(file, line);
        animation.length = std::stof(line);
        
        std::getline(file, line);
        uint32_t channel_count = std::stoi(line);
        for (uint32_t channel_index = 0; channel_index < channel_count; channel_index++) {
            std::getline(file, line);
            uint32_t bone_index = std::stoi(line);
            animation.channels[bone_index] = AnimationChannel {};

            std::getline(file, line);
            uint32_t position_keyframe_count = std::stoi(line);
            for (uint32_t position_keyframe_index = 0; position_keyframe_index < position_keyframe_count; position_keyframe_index++) {
                KeyframePosition keyframe;
                std::getline(file, line);
                keyframe.time = std::stof(line);
                std::getline(file, line);
                keyframe.position.x = std::stof(line);
                std::getline(file, line);
                keyframe.position.y = std::stof(line);
                std::getline(file, line);
                keyframe.position.z = std::stof(line);
                animation.channels[bone_index].position_keyframes.push_back(keyframe);
            }

            std::getline(file, line);
            uint32_t rotation_keyframe_count = std::stoi(line);
            for (uint32_t rotation_keyframe_index = 0; rotation_keyframe_index < rotation_keyframe_count; rotation_keyframe_index++) {
                KeyframeRotation keyframe;
                std::getline(file, line);
                keyframe.time = std::stof(line);
                std::getline(file, line);
                keyframe.rotation.x = std::stof(line);
                std::getline(file, line);
                keyframe.rotation.y = std::stof(line);
                std::getline(file, line);
                keyframe.rotation.z = std::stof(line);
                std::getline(file, line);
                keyframe.rotation.w = std::stof(line);
                animation.channels[bone_index].rotation_keyframes.push_back(keyframe);
            }
        }
        animation_library.animations[animation_name] = animation;
    }
    if (!animation_library.animations.empty()) {
        animation_player = std::make_shared<AnimationPlayer>();
        animation_player->library = animation_library;
    }

    // Meshes
    if (!mesh.loaded()) {
        std::println("Loading mesh: {}", mesh_guid.c_str());

        // Surfaces
        while (true) {
            std::getline(file, line);
            if (line == "") {
                break;
            }
            uint32_t start_index = std::stoi(line);
            std::getline(file, line);
            uint32_t count = std::stoi(line);
            std::getline(file, line);
            uint32_t material_index = std::stoi(line);
            mesh->surfaces.emplace_back(MeshSurface {
                start_index,
                count,
                materials[std::to_string(material_index)],
            });
        }

        // Vertex data
        std::getline(file, line);
        uint32_t vertex_count = std::stoi(line);
        std::getline(file, line);
        uint32_t index_count = std::stoi(line);
        std::getline(file, line);
        uint32_t skinning_data_count = std::stoi(line);
        std::getline(file, line);
        uint32_t joint_count = std::stoi(line);

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<SkinningData> skinning_data;
        std::vector<JointData> joint_data;

        vertices.resize(vertex_count);
        indices.resize(index_count);
        skinning_data.resize(skinning_data_count);
        joint_data.resize(joint_count);
        
        file.read(reinterpret_cast<char*>(vertices.data()), vertex_count * sizeof(Vertex));
        file.read(reinterpret_cast<char*>(indices.data()), index_count * sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(skinning_data.data()), skinning_data_count * sizeof(SkinningData));
        file.read(reinterpret_cast<char*>(joint_data.data()), joint_count * sizeof(JointData));

        auto skeleton_node = Node::create("Skeleton");
        auto skeleton = std::make_shared<Skeleton>();
        skeleton->joint_matrices.resize(joint_count);
        skeleton_node->add_component<Skeleton>(skeleton);
        
        skeleton->bones.resize(joint_count);
        target_skeleton = skeleton;

        std::vector<Mat4> joint_matrices;
        joint_matrices.resize(joint_count);
        for (int i = 0; i < joint_count; i++) {
            skeleton->bones[i] = Node::create("Bone");
            auto bone = std::make_shared<Bone>();
            bone->skeleton = skeleton;
            bone->index = i;
            bone->inverse_bind_matrix = joint_data[i].inverse_bind_matrix;
            skeleton->bones[i]->add_component<Bone>(bone);
            skeleton->bones[i]->set_position(joint_data[i].position);
            skeleton->bones[i]->set_rotation(joint_data[i].rotation);
            skeleton->bones[i]->set_scale(joint_data[i].scale);
            if (i == 20) {
                target_bone = bone;
            }
        }

        for (int i = 0; i < joint_count; i++) {
            if (joint_data[i].parent_index == -1) {
                skeleton_node->add_child(skeleton->bones[i]);
            } else {
                skeleton->bones[joint_data[i].parent_index]->add_child(skeleton->bones[i]);
            }
        }

        skeleton_node->refresh_transform(Transform());
        if (animation_player != nullptr) {
            animation_player->skeleton = skeleton;
        }

        for (int i = 0; i < joint_count; i++) {
            joint_matrices[i] = skeleton->bones[i]->get_global_transform().get_matrix() * joint_data[i].inverse_bind_matrix;
        }
        
        if (skinning_data.size() > 0) {
            mesh->mesh_buffers = gRenderer.upload_mesh(indices, vertices, skinning_data, joint_matrices);
            skeleton->joint_matrices_buffer = &(mesh->mesh_buffers.joint_matrices_buffer);
            node->add_child(skeleton_node);
        } else {
            mesh->mesh_buffers = gRenderer.upload_mesh(indices, vertices);
        }
        
        mesh->vertex_count = vertices.size();
        mesh.set_load_status(LoadStatus::LOADED);
    } else {
        std::println("Reusing mesh: {}", mesh_guid.c_str());
    }
    mesh.reference();

    file.close();

    // Nodes
    auto mesh_instance = node->add_component<MeshInstance>();
    mesh_instance->mesh = meshes["x"];
    node->refresh_transform(Transform());
}

std::string LoadedGLTF::get_name() {
    return "LoadedGLTF";
}

void import_gltf_scene(Renderer* p_renderer, std::filesystem::path p_file_path) {
    print("Importing GLTF: %s", p_file_path.c_str());
    auto target_path = std::format("{}.{}", p_file_path.c_str(), "imported");
    std::fstream file(target_path.c_str() , std::fstream::out | std::fstream::binary);
    
    constexpr auto gltf_options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers;
    auto data = fastgltf::GltfDataBuffer::FromPath(p_file_path);
    if (data.error() != fastgltf::Error::None) {
        print("Could not load GLTF!");
        return;
    }

    YAML::Node import_settings;
    std::string import_settings_path = std::format("{}.import", p_file_path.c_str());
    if (std::filesystem::exists(import_settings_path)) {
        import_settings = YAML::LoadFile(import_settings_path);
    }

    fastgltf::Asset asset;
    fastgltf::Parser parser {};
    auto type = fastgltf::determineGltfFileType(data.get());
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(data.get(), p_file_path.parent_path(), gltf_options);
        if (load.error() != fastgltf::Error::None) {
            print("Could not parse GLTF!");
            return;
        } else {
            asset = std::move(load.get());
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadGltfBinary(data.get(), p_file_path.parent_path(), gltf_options);
        if (load.error() != fastgltf::Error::None) {
            print("Could not parse GLTF!");
            return;
        } else {
            asset = std::move(load.get());
        }
    } else {
        print("Could not parse GLTF!");
        return;
    }

    std::vector<SamplerCreateInfo> sampler_infos;
    for (fastgltf::Sampler& sampler : asset.samplers) {
        sampler_infos.emplace_back(SamplerCreateInfo {
            .magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
            .minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
            .addressModeU = extract_address_mode(sampler.wrapS),
            .addressModeV = extract_address_mode(sampler.wrapT),
            .minLod = 0,
            .maxLod = LodClampNone,
        });
    }

    file << "Samplers\n";
    file << sampler_infos.size() << "\n";
    file.write((char*)sampler_infos.data(), sampler_infos.size() * sizeof(SamplerCreateInfo));
    
    // Textures
    file << "Textures\n";
    file << asset.images.size() << "\n";
    for (auto image : asset.images) {
        std::visit(
            fastgltf::visitor {
                [](auto& argument) {},
                [&](fastgltf::sources::URI& file_path) {
                    assert(file_path.fileByteOffset == 0);
                    assert(file_path.uri.isLocalPath());
                    auto full_path = p_file_path.parent_path().string() + "/" + file_path.uri.c_str();
                    auto ktx_path = std::format("{}.{}", full_path, "ktx2");
                    if (std::filesystem::exists(ktx_path)) {
                        full_path = ktx_path;
                    }
                    file << full_path << "\n";
                },
                [&](fastgltf::sources::Array& array) {
                    // TODO
                    file << "-1";
                },
                [&](fastgltf::sources::BufferView& view) {
                    // TODO
                    auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
                    auto& buffer = asset.buffers[buffer_view.bufferIndex];
                    std::visit(
                        fastgltf::visitor {
                            [](auto& argument) {},
                            [&](fastgltf::sources::Array& array) {
                                VkFormat format;
                                bool format_specified = false;
                                for (auto texture : import_settings["Textures"]) {
                                    if (texture["name"].as<std::string>() == image.name.c_str()) {
                                        format = texture["sRGB"].as<bool>() ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
                                        format_specified = true;
                                    }
                                }
                                if (!format_specified) {
                                    format = VK_FORMAT_R8G8B8A8_UNORM;
                                    YAML::Node texture_node {};
                                    texture_node["name"] = image.name.c_str();
                                    texture_node["sRGB"] = false;
                                    import_settings["Textures"].push_back(texture_node);
                                }

                                int width, height, number_channels;
                                unsigned char* data = stbi_load_from_memory((stbi_uc*)array.bytes.data() + buffer_view.byteOffset, static_cast<int>(buffer_view.byteLength), &width, &height, &number_channels, 4);
                                if (data == nullptr) {
                                    return;
                                }
                                ktxTexture2* texture;
                                KTX_error_code result;
                                ktx_uint32_t level, layer, face_slice;
                                level = layer = face_slice = 0;
                                ktxTextureCreateInfo ktx_info {
                                    .vkFormat = format,
                                    .baseWidth = (ktx_uint32_t)width,
                                    .baseHeight = (ktx_uint32_t)height,
                                    .baseDepth = 1,
                                    .numDimensions = 2,
                                    .numLevels = (ktx_uint32_t)1,//(std::log2(width) + 1),
                                    .numLayers = (ktx_uint32_t)1,
                                    .numFaces = (ktx_uint32_t)1,
                                    .isArray = KTX_FALSE,
                                    .generateMipmaps = KTX_TRUE,
                                };
                                result = ktxTexture2_Create(&ktx_info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &texture);
                                texture->pData = static_cast<uint8_t*>(data);

                                auto texture_path = std::format("{}/{}.ktx2", p_file_path.parent_path().c_str(), image.name.c_str());
                                ktxTexture_WriteToNamedFile(ktxTexture(texture), texture_path.c_str());
                                texture->pData = nullptr;
                                stbi_image_free(data);
                                file << texture_path << "\n";
                            }
                        }, buffer.data
                    );
                },
            }, image.data
        );
    }

    // Materials
    file << "Materials\n";
    file << asset.materials.size() << "\n";
    uint32_t data_index = 0;
    for (fastgltf::Material& temp_material : asset.materials) {
        
        file << (float)temp_material.pbrData.baseColorFactor[0] << "\n";
        file << (float)temp_material.pbrData.baseColorFactor[1] << "\n";
        file << (float)temp_material.pbrData.baseColorFactor[2] << "\n";
        file << (float)temp_material.pbrData.baseColorFactor[3] << "\n";
        file << (float)temp_material.pbrData.metallicFactor << "\n";
        file << (float)temp_material.pbrData.roughnessFactor << "\n";

        if (temp_material.pbrData.baseColorTexture.has_value()) {
            size_t image_index   = asset.textures[temp_material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler_index = asset.textures[temp_material.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            file << image_index << "\n";
            file << sampler_index << "\n";
        } else {
            file << -1 << "\n";
            file << -1 << "\n";
        }

        if (temp_material.normalTexture.has_value()) {
            size_t image_index   = asset.textures[temp_material.normalTexture.value().textureIndex].imageIndex.value();
            size_t sampler_index = asset.textures[temp_material.normalTexture.value().textureIndex].samplerIndex.value();

            file << image_index << "\n";
            file << sampler_index << "\n";
        } else {
            file << -1 << "\n";
            file << -1 << "\n";
        }

        if (temp_material.pbrData.metallicRoughnessTexture.has_value()) {
            size_t image_index   = asset.textures[temp_material.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
            size_t sampler_index = asset.textures[temp_material.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();

            file << image_index << "\n";
            file << sampler_index << "\n";
        } else {
            file << -1 << "\n";
            file << -1 << "\n";
        }
    }

    // Skin
    std::vector<JointData> joint_data;
    if (!asset.skins.empty()) {
        auto inverse_bind_matrix_accessor_index = asset.skins[0].inverseBindMatrices;
        auto inverse_bind_matrix_accessor = asset.accessors[inverse_bind_matrix_accessor_index.value()];
        joint_data.resize(inverse_bind_matrix_accessor.count);
        fastgltf::iterateAccessorWithIndex<Mat4>(asset, inverse_bind_matrix_accessor,
            [&](Mat4 inverse_bind_matrix, size_t index) {
                auto node_index = asset.skins[0].joints[index];
                joint_data[node_index].inverse_bind_matrix = inverse_bind_matrix;
                joint_data[index].parent_index = -1;
            }
        );
        for (uint32_t joint_index = 0; joint_index < asset.skins[0].joints.size(); joint_index++) {
            auto node_index = asset.skins[0].joints[joint_index];
            auto joint = asset.nodes[node_index];
            auto transform = std::get<fastgltf::TRS>(joint.transform);
            
            joint_data[node_index].position = Vec3(transform.translation[0], transform.translation[1], transform.translation[2]);
            joint_data[node_index].rotation = glm::quat(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
            joint_data[node_index].scale = transform.scale[0];
            for (auto child_index : joint.children) {
                joint_data[child_index].parent_index = node_index;
            }
        }
    }

    // Animations
    file << "Animations\n";
    file << asset.animations.size() << "\n";
    AnimationLibrary animation_library;
    if (!asset.animations.empty()) {
        for (auto& gltf_animation : asset.animations) {
            file << gltf_animation.name.c_str() << "\n";
            float animation_length = 0.0f;
            std::unordered_map<uint32_t, AnimationChannel> channels;
            for (auto gltf_channel : gltf_animation.channels) {
                auto animation_sampler = gltf_animation.samplers[gltf_channel.samplerIndex];
                auto input_accessor = asset.accessors[animation_sampler.inputAccessor];
                auto output_accessor = asset.accessors[animation_sampler.outputAccessor];
                if (channels.find(gltf_channel.nodeIndex.value()) == channels.end()) {
                    channels[gltf_channel.nodeIndex.value()] = AnimationChannel {};
                }
                auto channel = &channels[gltf_channel.nodeIndex.value()];
                if (gltf_channel.path == fastgltf::AnimationPath::Translation) {
                    fastgltf::iterateAccessorWithIndex<float>(asset, input_accessor,
                        [&](float p_time, size_t index) {
                            animation_length = std::max(animation_length, p_time);
                            KeyframePosition keyframe;
                            keyframe.time = p_time;
                            channel->position_keyframes.push_back(keyframe);
                        }
                    );
                    fastgltf::iterateAccessorWithIndex<Vec3>(asset, output_accessor,
                        [&](Vec3 p_position, size_t index) {
                            channel->position_keyframes[index].position = p_position;
                        }
                    );
                } else if (gltf_channel.path == fastgltf::AnimationPath::Rotation) {
                    fastgltf::iterateAccessorWithIndex<float>(asset, input_accessor,
                        [&](float p_time, size_t index) {
                            animation_length = std::max(animation_length, p_time);
                            KeyframeRotation keyframe;
                            keyframe.time = p_time;
                            channel->rotation_keyframes.push_back(keyframe);
                        }
                    );
                    fastgltf::iterateAccessorWithIndex<Vec4>(asset, output_accessor,
                        [&](Vec4 p_rotation, size_t index) {
                            channel->rotation_keyframes[index].rotation = Quaternion(p_rotation[3], p_rotation[0], p_rotation[1], p_rotation[2]);
                        }
                    );
                }
            }
            file << animation_length << "\n";
            file << channels.size() << "\n";
            for (auto& kv : channels) {
                file << kv.first << "\n"; // node index
                file << kv.second.position_keyframes.size() << "\n";
                for (auto& keyframe : kv.second.position_keyframes) {
                    file << keyframe.time << "\n";
                    file << keyframe.position.x << "\n";
                    file << keyframe.position.y << "\n";
                    file << keyframe.position.z << "\n";
                }
                file << kv.second.rotation_keyframes.size() << "\n";
                for (auto& keyframe : kv.second.rotation_keyframes) {
                    file << keyframe.time << "\n";
                    file << keyframe.rotation.x << "\n";
                    file << keyframe.rotation.y << "\n";
                    file << keyframe.rotation.z << "\n";
                    file << keyframe.rotation.w << "\n";
                }
            }
        }
    }

    // Meshes
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<SkinningData> skinning_data;

    for (fastgltf::Mesh& mesh : asset.meshes) {
        indices.clear();
        vertices.clear();
        skinning_data.clear();

        for (auto&& primitive : mesh.primitives) {
            // Surfaces
            MeshSurface new_surface;
            new_surface.start_index = (uint32_t) indices.size();
            new_surface.count = (uint32_t) asset.accessors[primitive.indicesAccessor.value()].count;

            // Vertices and indices
            size_t initial_vertex_index = vertices.size();

            // Indices
            {
                fastgltf::Accessor& index_accessor = asset.accessors[primitive.indicesAccessor.value()];
                indices.reserve(indices.size() + index_accessor.count);
                fastgltf::iterateAccessor<std::uint32_t>(asset, index_accessor,
                    [&](std::uint32_t index) {
                        indices.push_back(initial_vertex_index + index);
                    }
                );
            }

            // Positions
            {
                fastgltf::Accessor& position_accessor = asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + position_accessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, position_accessor,
                    [&](glm::vec3 vector, size_t index) {
                        Vertex new_vertex {
                            .position = vector,
                            .uv_x = 0.0f,
                            .normal = { 1.0f, 0.0f, 0.0f },
                            .uv_y = 0.0f,
                            .color = Vec4(1.0f),
                        };
                        vertices[initial_vertex_index + index] = new_vertex;
                    }
                );
            }

            // Normals
            auto normals = primitive.findAttribute("NORMAL");
            if (normals != primitive.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<Vec3>(asset, asset.accessors[(*normals).accessorIndex],
                    [&](Vec3 normal, size_t index) {
                        vertices[initial_vertex_index + index].normal = glm::normalize(normal);
                    }
                );
            }

            // Tangents
            auto tangents = primitive.findAttribute("TANGENT");
            if (tangents != primitive.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<Vec4>(asset, asset.accessors[(*tangents).accessorIndex],
                    [&](Vec4 tangent, size_t index) {
                        vertices[initial_vertex_index + index].tangent = tangent;
                    }
                );
            }

            // UV
            auto tex_coord = primitive.findAttribute("TEXCOORD_0");
            if (tex_coord != primitive.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<Vec2>(asset, asset.accessors[(*tex_coord).accessorIndex],
                    [&](Vec2 uv, size_t index) {
                        vertices[initial_vertex_index + index].uv_x = uv.x;
                        vertices[initial_vertex_index + index].uv_y = uv.y;
                    }
                );
            }

            // Color
            auto color_attribute = primitive.findAttribute("COLOR_1");
            if (color_attribute != primitive.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<Vec4>(asset, asset.accessors[(*color_attribute).accessorIndex],
                    [&](Vec4 color, size_t index) {
                        vertices[initial_vertex_index + index].color = color;
                    }
                );
            }
            
            // Joints
            auto joint_attribute = primitive.findAttribute(std::format("JOINTS_{}", 0));
            if (joint_attribute != primitive.attributes.end()) {
                skinning_data.resize(vertices.size());
                fastgltf::Accessor& joint_accessor = asset.accessors[joint_attribute->accessorIndex];
                fastgltf::iterateAccessorWithIndex<Vec4>(asset, joint_accessor,
                    [&](Vec4 joint, size_t index) {
                        skinning_data[initial_vertex_index + index] = SkinningData {
                            .joint_ids = {
                                uint32_t(asset.skins[0].joints[uint32_t(joint.x)]),
                                uint32_t(asset.skins[0].joints[uint32_t(joint.y)]),
                                uint32_t(asset.skins[0].joints[uint32_t(joint.z)]),
                                uint32_t(asset.skins[0].joints[uint32_t(joint.w)]),
                            }
                        };
                    }
                );

                auto weights_attribute = primitive.findAttribute(std::format("WEIGHTS_{}", 0));
                assert(weights_attribute != primitive.attributes.end());
                fastgltf::Accessor& weight_accessor = asset.accessors[weights_attribute->accessorIndex];
                fastgltf::iterateAccessorWithIndex<Vec4>(asset, weight_accessor,
                    [&](Vec4 weights, size_t index) {
                        skinning_data[initial_vertex_index + index].weights = weights;
                    }
                );
            }

            file << new_surface.start_index << "\n";
            file << new_surface.count << "\n";
            if (primitive.materialIndex.has_value()) {
                file << primitive.materialIndex.value() << "\n";
            } else {
                file << -1 << "\n";
            }
        }

        file << "\n";
        file << vertices.size() << "\n";
        file << indices.size() << "\n";
        file << skinning_data.size() << "\n";
        file << joint_data.size() << "\n";

        file.write((char*)vertices.data(), vertices.size() * sizeof(Vertex));
        file.write((char*)indices.data(), indices.size() * sizeof(uint32_t));
        file.write((char*)skinning_data.data(), skinning_data.size() * sizeof(SkinningData));
        file.write((char*)joint_data.data(), joint_data.size() * sizeof(JointData));
    }

    file.close();

    std::ofstream import_settings_output_stream(import_settings_path.c_str());
    
    import_settings_output_stream << import_settings;
    import_settings_output_stream.close();
}

// TODO: Do this somewhere else
namespace YAML {
    template<>
    struct convert<Vec3> {
        static Node encode(const Vec3& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node& node, Vec3& rhs) {
            if (!node.IsSequence() || node.size() != 3) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<Vec4> {
        static Node encode(const Vec4& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            return node;
        }

        static bool decode(const Node& node, Vec4& rhs) {
            if (!node.IsSequence() || node.size() != 4) {
                return false;
            }

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };
}

std::shared_ptr<Node> load_scene(std::string p_path) {
    YAML::Node scene_file = YAML::LoadFile(p_path);
    const std::string name = scene_file["name"] ? scene_file["name"].as<std::string>() : "Node";
    auto scene = Node::create(name);

    const float scale = scene_file["scale"] ? scene_file["scale"].as<float>() : 1.0f;
    const Vec3 position = scene_file["position"] ? scene_file["position"].as<Vec3>() : Vec3();
    scene->set_position(position);

    if (scene_file["model"]) {
        auto model_data = std::make_shared<LoadedGLTF>();
        
        auto model_node = Node::create(name);
        scene->add_child(model_node);
        model_node->add_component<LoadedGLTF>(model_data);
        model_data->renderer = &gRenderer;

        const std::string model_path = scene_file["model"].as<std::string>();
        std::string imported_path = std::format("{}.imported", model_path);
        // TODO: Check if file changed
        if (!std::filesystem::exists(imported_path)) {
            import_gltf_scene(&gRenderer, model_path);
        }
        model_data->load(imported_path);
        model_node->set_scale(scale);

        if (scene_file["skinned"]) {
            auto skinning = std::make_shared<SkinnedMesh>();
            skinning->mesh = model_node->get_component<MeshInstance>()->mesh;
            skinning->mesh->reference();
            model_node->add_component<SkinnedMesh>(skinning);
        }
    }
    
    for (size_t index = 0; index < scene_file["children"].size(); index++) {
        const YAML::Node child_node = scene_file["children"][index];
        const std::string child_name = child_node["name"].as<std::string>();
        auto new_node = Node::create(child_name);

        const Vec3 position = child_node["position"] ? child_node["position"].as<Vec3>() : Vec3();
        const float scale = child_node["scale"] ? child_node["scale"].as<float>() : 1.0f;
        new_node->set_position(position);

        if (child_node["point_light"]) {
            auto light_component = std::make_shared<PointLight>();
            light_component->color     = child_node["point_light"]["color"].as<Vec3>();
            light_component->intensity = child_node["point_light"]["intensity"].as<float>();
            new_node->add_component<PointLight>(light_component);
        }

        if (child_node["scene"]) {
            const std::string subscene_path = child_node["scene"].as<std::string>();
            auto subscene = load_scene(subscene_path);
            subscene->scale_by(scale);
            new_node->add_child(subscene);
        }
        
        scene->add_child(new_node);
    }

    return scene;
}

std::tuple<std::vector<Vec3>, std::vector<uint>> load_triangles(std::filesystem::path p_file_path) {
    print("Loading GLTF vertices: %s", p_file_path.c_str());

    fastgltf::Parser parser {};

    constexpr auto gltf_options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadExternalBuffers;
    auto data = fastgltf::GltfDataBuffer::FromPath(p_file_path);
    if (data.error() != fastgltf::Error::None) {
        print("Could not load GLTF!");
        return {};
    }

    fastgltf::Asset asset;

    auto type = fastgltf::determineGltfFileType(data.get());
    if (type == fastgltf::GltfType::glTF) {
        auto load = parser.loadGltf(data.get(), p_file_path.parent_path(), gltf_options);
        if (load.error() != fastgltf::Error::None) {
            print("Could not parse GLTF!");
            return {};
        } else {
            asset = std::move(load.get());
        }
    } else if (type == fastgltf::GltfType::GLB) {
        auto load = parser.loadGltfBinary(data.get(), p_file_path.parent_path(), gltf_options);
        if (load.error() != fastgltf::Error::None) {
            print("Could not parse GLTF!");
            return {};
        } else {
            asset = std::move(load.get());
        }
    } else {
        print("Could not parse GLTF!");
        return {};
    }

    // Meshes
    std::vector<uint32_t> indices;
    std::vector<Vec3> vertices;

    for (fastgltf::Mesh& mesh : asset.meshes) {
        indices.clear();
        vertices.clear();

        for (auto&& primitive : mesh.primitives) {
            // Vertices and indices
            size_t initial_vertex_index = vertices.size();

            // Indices
            {
                fastgltf::Accessor& index_accessor = asset.accessors[primitive.indicesAccessor.value()];
                indices.resize(indices.size() + index_accessor.count);
                fastgltf::iterateAccessor<std::uint32_t>(asset, index_accessor,
                    [&](std::uint32_t index) {
                        indices.push_back(index + initial_vertex_index);
                    }
                );
            }

            // Positions
            {
                fastgltf::Accessor& position_accessor = asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + position_accessor.count);
                fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, position_accessor,
                    [&](glm::vec3 vector, size_t index) {
                        vertices[initial_vertex_index + index] = 0.0104f * vector;
                    }
                );
            }
        }
    }

    return {vertices, indices};
}