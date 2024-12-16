#include <core/scene_graph.h>
#include <core/node.h>

void SceneGraph::update(double delta) {
    point_lights.clear();
    root->update(delta);
}