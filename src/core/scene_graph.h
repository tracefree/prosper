#pragma once

#include <util.h>

struct Camera;
struct Node;
struct GPUPointLight;

struct SceneGraph {
    Ref<Node> root;
    Ref<Node> camera;

    std::vector<Ref<Node>> leafs;
    std::vector<GPUPointLight> point_lights;

    void update(double delta);
    void cleanup();
};