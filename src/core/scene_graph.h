#pragma once

#include <rendering/types.h>
#include <memory>

struct Camera;
struct Node;

struct SceneGraph {
    std::shared_ptr<Node> root;

    std::vector<std::shared_ptr<Node>> leafs;
    std::vector<GPUPointLight> point_lights;
    std::shared_ptr<Camera> camera;

    void update(double delta);
};