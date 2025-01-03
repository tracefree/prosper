#include <resources/scene.h>

#include <core/node.h>
#include <rendering/renderer.h>
#include <core/resource_manager.h>
#include <loader.h>
#include <yaml.h>
#include <components/skinned_mesh.h>
#include <components/mesh_instance.h>
#include <components/physics/static_body.h>
#include <resources/scene.h>
#include <components/bone.h>
#include <components/skeleton.h>
#include <components/point_light.h>
#include <components/model_data.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

extern Renderer gRenderer; // TODO remove


template<>
std::shared_ptr<Resource<Scene>> ResourceManager::load<Scene>(const char* p_guid) {
    auto& resource = (*ResourceManager::get<Scene>(p_guid));
    resource->scene_state = std::make_shared<YAML::Node>(YAML::LoadFile(p_guid));
    resource.set_load_status(LoadStatus::LOADED);
    return ResourceManager::get<Scene>(p_guid);
}

std::shared_ptr<Node> Scene::instantiate() {
    if (scene_state == nullptr) {
        std::println("Error: No state");
        return std::make_shared<Node>();
    }

    return from_data(*scene_state);
}

std::shared_ptr<Node> Scene::from_data(YAML::Node p_data) {
    auto scene = Node::create();
    
    if (p_data["name"]) {
        scene->name = p_data["name"].as<std::string>();
    }

    if (p_data["position"]) {
        scene->set_position(p_data["position"].as<Vec3>());
    }

    if (p_data["scale"]) {
        scene->scale_by(p_data["scale"].as<float>());
    }

    /*
    if (p_data["model"]) {
        auto model_data = std::make_shared<ModelData>();
        
        auto model_node = Node::create(scene->name);
        scene->add_child(model_node);
        model_node->add_component<ModelData>(model_data);
        model_data->renderer = &gRenderer;

        const std::string model_path = p_data["model"].as<std::string>();
        std::string imported_path = std::format("{}.imported", model_path);
        // TODO: Check if file changed
        if (!std::filesystem::exists(imported_path)) {
            import_gltf_scene(&gRenderer, model_path);
        }
        model_data->load(imported_path);
        model_node->set_scale(scene->get_scale());

        if (p_data["skinned"]) {
            auto skinning = std::make_shared<SkinnedMesh>();
            skinning->mesh = model_node->get_component<MeshInstance>()->mesh;
            skinning->mesh->reference();
            model_node->add_component<SkinnedMesh>(skinning);
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
                    auto static_body = scene->add_component<StaticBody>();
                    static_body->shape = restored_shape.GetPtr();
                    static_body->initialize();
                }
            }
        }
    }
    */
    for (size_t index = 0; index < p_data["components"].size(); index++) {
        const YAML::Node component_node = p_data["components"][index];
        auto component_name = component_node["type"].as<std::string>();
        Component::create_functions[component_name](component_node, scene);
    }

    for (size_t index = 0; index < p_data["children"].size(); index++) {
        std::shared_ptr<Node> new_node;

        const YAML::Node child_node = p_data["children"][index];
        if (child_node["scene"]) {
            const std::string subscene_path = child_node["scene"].as<std::string>();
            new_node = (*ResourceManager::load<Scene>(subscene_path.c_str()))->instantiate();
        } else {
            new_node = from_data(child_node);
        }

        scene->add_child(new_node);
    }

    return scene;
}