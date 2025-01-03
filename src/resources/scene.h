#pragma once
#include <memory>

struct Node;
namespace YAML {
    class Node;
}

struct Scene {
    std::shared_ptr<YAML::Node> scene_state;

    std::shared_ptr<Node> from_data(YAML::Node p_data);
    std::shared_ptr<Node> instantiate();
};