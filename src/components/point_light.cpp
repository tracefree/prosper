#include <components/point_light.h>
#include <core/node.h>
#include <core/scene_graph.h>
#include <rendering/types.h>

#include <yaml.h>

extern SceneGraph scene; // TODO: don't

void PointLight::update(double _delta) {
    scene.point_lights.emplace_back(
        GPUPointLight {
            .position = node->get_global_transform().position,
            .intensity = intensity,
            .color = color,
        }
    );
}

COMPONENT_FACTORY_IMPL(PointLight, point_light) {
    color     = p_data["color"].as<Vec3>();
    intensity = p_data["intensity"].as<float>();
}