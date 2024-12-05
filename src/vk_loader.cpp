#include "vk_loader.h"

#include "renderer.h"
#include "util.h"

#include "stb_image.h"
#include <iostream>
#include <variant>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

using namespace vk;

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

std::optional<std::shared_ptr<LoadedGLTF>> load_gltf_scene(Renderer* p_renderer, std::filesystem::path p_file_path) {
    print("Loading GLTF: %s", p_file_path.c_str());
    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    scene->renderer = p_renderer;
    LoadedGLTF& file = *scene.get();

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

    

    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        { DescriptorType::eCombinedImageSampler, 3},
        { DescriptorType::eUniformBuffer, 3},
        { DescriptorType::eStorageBuffer, 1}
    };

    file.descriptor_pool.init_pool(p_renderer->device, asset.materials.size(), sizes);

    for (fastgltf::Sampler& sampler : asset.samplers) {
        SamplerCreateInfo sampler_info {
            .magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest)),
            .minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest)),
        //    .addressModeU = SamplerAddressMode::eClampToEdge,
        //    .addressModeV = SamplerAddressMode::eClampToEdge,
        //    .addressModeW = SamplerAddressMode::eClampToEdge,
            .minLod = 0,
            .maxLod = LodClampNone,
            
        };

        Result result;
        Sampler new_sampler;
        std::tie(result, new_sampler) = p_renderer->device.createSampler(sampler_info);

        file.samplers.push_back(new_sampler);
    }

    std::vector<AllocatedImage> temp_textures;
    std::vector<std::shared_ptr<MaterialInstance>> temp_materials;
    std::vector<std::shared_ptr<MeshAsset>> temp_meshes;
    std::vector<std::shared_ptr<Node>> temp_nodes;
    

    // Textures

    uint32_t texture_index {0};
    for (fastgltf::Image& image : asset.images) {
        std::optional<AllocatedImage> new_image = load_image(p_renderer, asset, image);

        if (new_image.has_value()) {
            temp_textures.push_back(*new_image);
            std::string texture_id = std::to_string(texture_index);
            file.textures[texture_id.c_str()] = *new_image;
        } else {
            temp_textures.push_back(p_renderer->image_error);
            print("Failed to load texture: %s", image.name.c_str());
        }

        texture_index++;
    }

    file.material_data_buffer = p_renderer->create_buffer(
        sizeof(MaterialMetallicRoughness::MaterialConstants) * asset.materials.size(),
        BufferUsageFlagBits::eUniformBuffer,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );

    // Materials

    uint32_t data_index = 0;
    MaterialMetallicRoughness::MaterialConstants* scene_material_constants = (MaterialMetallicRoughness::MaterialConstants*) file.material_data_buffer.info.pMappedData;

    for (fastgltf::Material& temp_material : asset.materials) {
        std::shared_ptr<MaterialInstance> new_material = std::make_shared<MaterialInstance>();
        temp_materials.push_back(new_material);
        file.materials[temp_material.name.c_str()] = new_material;

        MaterialMetallicRoughness::MaterialConstants constants {
            .albedo_factors = Vec4(
                (float)temp_material.pbrData.baseColorFactor[0],
                (float)temp_material.pbrData.baseColorFactor[1],
                (float)temp_material.pbrData.baseColorFactor[2],
                (float)temp_material.pbrData.baseColorFactor[3]
            ),
            .metal_roughness_factors = Vec4(
                (float)temp_material.pbrData.metallicFactor,
                (float)temp_material.pbrData.roughnessFactor,
                0.0f, 0.0f
            )
        };
        scene_material_constants[data_index] = constants;

        MaterialPass pass_type = (temp_material.alphaMode == fastgltf::AlphaMode::Blend) ? MaterialPass::Transparent : MaterialPass::MainColor;
        PhysicalDeviceProperties physical_device_properties = p_renderer->physical_device.getProperties();
        
        uint32_t min_offset_multiplier = ceilf(float(sizeof(MaterialMetallicRoughness::MaterialResources)) / float(physical_device_properties.limits.minUniformBufferOffsetAlignment));

        MaterialMetallicRoughness::MaterialResources material_resources {
            .albedo_image = p_renderer->image_white,
            .albedo_sampler = p_renderer->sampler_default_nearest,
            .metal_roughness_image = p_renderer->image_black,
            .metal_roughness_sampler = p_renderer->sampler_default_linear,
            .data_buffer = file.material_data_buffer.buffer,
            .data_buffer_offset = data_index * min_offset_multiplier * uint32_t(physical_device_properties.limits.minUniformBufferOffsetAlignment),
        };

        if (temp_material.pbrData.baseColorTexture.has_value()) {
            size_t image_index   = asset.textures[temp_material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler_index = asset.textures[temp_material.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            material_resources.albedo_image = temp_textures[image_index];
            material_resources.albedo_sampler = file.samplers[sampler_index];
        }

        *new_material = p_renderer->metal_roughness_material.write_material(p_renderer->device, pass_type, material_resources, file.descriptor_pool);
        data_index++;
    }

    // Meshes

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh& mesh : asset.meshes) {
        std::shared_ptr<MeshAsset> new_mesh = std::make_shared<MeshAsset>();
        temp_meshes.push_back(new_mesh);
        file.meshes[mesh.name.c_str()] = new_mesh;
        new_mesh->name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto&& primitive : mesh.primitives) {
            MeshSurface new_surface;
            new_surface.start_index = (uint32_t) indices.size();
            new_surface.count = (uint32_t) asset.accessors[primitive.indicesAccessor.value()].count;

            size_t initial_vertex_index = vertices.size();

            // Indices
            {
                fastgltf::Accessor& index_accessor = asset.accessors[primitive.indicesAccessor.value()];
                indices.reserve(indices.size() + index_accessor.count);
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
            auto color_attribute = primitive.findAttribute("COLOR_0");
            if (color_attribute != primitive.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<Vec4>(asset, asset.accessors[(*color_attribute).accessorIndex],
                    [&](Vec4 color, size_t index) {
                        vertices[initial_vertex_index + index].color = color;
                    }
                );
            }

            if (primitive.materialIndex.has_value()) {
                new_surface.material = temp_materials[primitive.materialIndex.value()];
            } else {
                new_surface.material = temp_materials[0];
            }

            new_mesh->surfaces.push_back(new_surface);
        }

        new_mesh->mesh_buffers = p_renderer->upload_mesh(indices, vertices);
    }

    // Nodes

    for (fastgltf::Node& node : asset.nodes) {
        std::shared_ptr<Node> new_node;
        
        if (node.meshIndex.has_value()) {
            new_node = std::make_shared<MeshInstance>();
            static_cast<MeshInstance*>(new_node.get())->mesh = temp_meshes[*node.meshIndex];
        } else {
            new_node = std::make_shared<Node>();
        }

        temp_nodes.push_back(new_node);
        file.nodes[node.name.c_str()];
        
        std::visit(fastgltf::visitor {
            [&](fastgltf::math::fmat4x4 matrix) {
                memcpy(&new_node->transform, matrix.data(), sizeof(matrix));
            },
            [&](fastgltf::TRS p_transform) {
                Vec3 translation(p_transform.translation[0], p_transform.translation[1], p_transform.translation[2]);
                glm::quat rotation(p_transform.rotation[3], p_transform.rotation[0], p_transform.rotation[1], p_transform.rotation[2]);
                Vec3 scale(p_transform.scale[0], p_transform.scale[1], p_transform.scale[2]);

                Mat4 translation_matrix = glm::translate(Mat4(1.0f), translation);
                Mat4 rotation_matrix = glm::toMat4(rotation);
                Mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);

                new_node->transform = translation_matrix * rotation_matrix * scale_matrix;
            }}, node.transform
        );
    }

    for (uint32_t i = 0; i< asset.nodes.size(); i++) {
        fastgltf::Node& node = asset.nodes[i];
        std::shared_ptr<Node>& scene_node = temp_nodes[i];
        
        for (auto& child : node.children) {
            scene_node->children.push_back(temp_nodes[child]);
            temp_nodes[child]->parent = scene_node;
        }
    }

    for (auto& node : temp_nodes) {
        if (node->parent.lock() == nullptr) {
            file.root_nodes.push_back(node);
            node->refresh_transform(Mat4(1.0f));
        }
    }

    return scene;
}


std::optional<AllocatedImage> load_image(Renderer* p_renderer, std::filesystem::path p_file_path) {
    AllocatedImage new_image {};

    int w, h, nrChannels;
    unsigned char* data = stbi_load(p_file_path.c_str(), &w, &h, &nrChannels, 4);
    new_image = p_renderer->create_image(data, Extent3D {uint32_t(w), uint32_t(h), 1}, Format::eR8G8B8A8Unorm, ImageUsageFlagBits::eSampled, false);
    return new_image;
}


std::optional<AllocatedImage> load_image(Renderer* p_renderer, fastgltf::Asset& p_asset, fastgltf::Image& p_image) {
    AllocatedImage new_image {};
    int width, height, number_channels;
    std::visit(
        fastgltf::visitor {
            [](auto& argument) {},
            [&](fastgltf::sources::URI& file_path) {
                assert(file_path.fileByteOffset == 0);
                assert(file_path.uri.isLocalPath());
                // TODO: find correct folder for texture assets relative to gltf file instead of hardcoding
                auto full_path = std::string("../../assets/models/Sponza/glTF/") + file_path.uri.c_str();
                unsigned char* data = stbi_load(full_path.c_str(), &width, &height, &number_channels, 4);
                if (data) {
                    Extent3D image_size {
                        .width  = uint32_t(width),
                        .height = uint32_t(height),
                        .depth  = 1,
                    };
                    new_image = p_renderer->create_image(data, image_size, Format::eR8G8B8A8Unorm, ImageUsageFlagBits::eSampled, false);
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
                    new_image = p_renderer->create_image(data, image_size, Format::eR8G8B8A8Unorm, ImageUsageFlagBits::eSampled, false);
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
                                new_image = p_renderer->create_image(data, image_size, Format::eR8G8B8A8Unorm, ImageUsageFlagBits::eSampled, false);
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

void LoadedGLTF::draw(const Mat4& p_transform, DrawContext& p_context) {
    for (auto& node : root_nodes) {
        node->draw(p_transform, p_context);
    }
}

void LoadedGLTF::clear_all() {
    Device device = renderer->device;
    descriptor_pool.destroy_pools(device);
    renderer->destroy_buffer(material_data_buffer);

    for (auto& [key, value] : meshes) {
        renderer->destroy_buffer(value->mesh_buffers.index_buffer);
        renderer->destroy_buffer(value->mesh_buffers.vertex_buffer);
    }

    for (auto& [key, value] : textures) {
        if (value.image == renderer->image_error.image) { continue; }
        if (value.image == renderer->image_white.image) { continue; }
        if (value.image == renderer->image_black.image) { continue; }
        renderer->destroy_image(value);
    }

    for (auto& sampler : samplers) {
        device.destroySampler(sampler);
    }
}