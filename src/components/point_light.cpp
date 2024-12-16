#include <components/point_light.h>
#include <core/node.h>
#include <core/scene_graph.h>

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

std::string PointLight::get_name() {
    return "PointLight";
}