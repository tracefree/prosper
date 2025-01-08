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