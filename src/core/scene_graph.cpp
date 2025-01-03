#include <core/scene_graph.h>
#include <core/node.h>
#include <components/camera.h>
#include <rendering/types.h>

void SceneGraph::update(double delta) {
    point_lights.clear();
    root->update(delta);
}

void SceneGraph::cleanup() {
    leafs.clear();
    point_lights.clear();
    camera = nullptr;
    root->cleanup();
    root = nullptr;
}