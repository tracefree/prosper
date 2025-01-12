#pragma once
#include <memory>
#include <util.h>

struct Node;
namespace YAML {
    class Node;
}

struct Scene {
    Ref<YAML::Node> scene_state;

    Ref<Node> from_data(YAML::Node p_data);
    Ref<Node> instantiate();
};