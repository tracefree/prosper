#include <core/component.h>

std::unordered_map<std::string, void(*)(YAML::Node, Ref<Node>)> Component::create_functions;

bool Component::register_type(const std::string p_name, void(*f)(YAML::Node p_data, Ref<Node> p_node)) {
    if (Component::create_functions.contains(p_name)) {
        return false;
    }

    Component::create_functions[p_name] = f;
    return true;
}