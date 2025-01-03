#include <components/model_data.h>

#include <core/resource_manager.h>
#include <core/node.h>
#include <components/bone.h>
#include <components/mesh_instance.h>
#include <components/skinned_mesh.h>
#include <components/skeleton.h>
#include <components/physics/static_body.h>
#include <rendering/renderer.h>
#include <resources/animation.h>

#include <yaml.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

extern Renderer gRenderer;

void ModelData::cleanup() {
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

void ModelData::initialize() {
    std::string imported_path = std::format("{}.imported", model_path);
    // TODO: Check if file changed
    if (!std::filesystem::exists(imported_path)) {
        import_gltf_scene(&gRenderer, model_path);
    }

    print("Loading imported GLTF: %s", imported_path.c_str());
    std::fstream file(imported_path, std::fstream::in | std::fstream::binary);
    
    auto mesh_guid = std::format("{}::/meshes/{}", imported_path.c_str(), "x");
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

        if (joint_count > 0) {
            auto skeleton_node = Node::create("Skeleton");
            skeleton = std::make_shared<Skeleton>();
            if (root_motion_index >= 0) {
                skeleton->root_motion_index = root_motion_index;
            }
            skeleton->joint_matrices.resize(joint_count);
            skeleton_node->add_component<Skeleton>(skeleton);
            
            skeleton->bones.resize(joint_count);
            
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
            }

            for (int i = 0; i < joint_count; i++) {
                if (joint_data[i].parent_index == -1) {
                    skeleton_node->add_child(skeleton->bones[i]);
                } else {
                    skeleton->bones[joint_data[i].parent_index]->add_child(skeleton->bones[i]);
                }
            }

            skeleton_node->refresh_transform(Transform());

            for (int i = 0; i < joint_count; i++) {
                joint_matrices[i] = skeleton->bones[i]->get_global_transform().get_matrix() * joint_data[i].inverse_bind_matrix;
            }

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

    if (skinned) {
        auto skinning = node->add_component<SkinnedMesh>();
        skinning->mesh = mesh_instance->mesh;
        skinning->mesh->reference();
    }

    auto collision_path = std::format("{}.col", model_path);
    if (std::filesystem::exists(collision_path)) {
        std::ifstream collision_file(collision_path, std::fstream::binary);
        JPH::StreamInWrapper stream_in(collision_file);
        JPH::Shape::IDToShapeMap id_to_shape;
        JPH::Shape::IDToMaterialMap id_to_material;
        if (collision_file.is_open()) {
            uint code;
            stream_in.Read(code);
            auto result = JPH::MeshShape::sRestoreFromBinaryState(stream_in);
            collision_file.close();
            JPH::Ref<JPH::Shape> restored_shape;
            std::println("Valid? {}", result.IsValid());
            if (result.IsValid()) {
                restored_shape = result.Get();
                auto static_body = node->add_component<StaticBody>();
                static_body->shape = restored_shape.GetPtr();
                static_body->initialize();
            }
        }
    }
}

COMPONENT_FACTORY_IMPL(ModelData, model) {
    renderer = &gRenderer;
    model_path = p_data["path"].as<std::string>();
    if (p_data["skinned"]) {
        skinned = p_data["skinned"].as<bool>();
    }
    if (p_data["root_motion_index"]) {
        root_motion_index = p_data["root_motion_index"].as<int>();
    }
}