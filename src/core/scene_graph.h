#pragma once

#include <rendering/types.h>
#include <memory>

struct Camera;
struct Node;

struct SceneGraph {
    std::shared_ptr<Node> root;
    std::shared_ptr<Node> camera;

    std::vector<std::shared_ptr<Node>> leafs;
    std::vector<GPUPointLight> point_lights;

    void update(double delta);
    void cleanup();
};