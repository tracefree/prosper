#pragma once
#include <math.h>
#include <util.h>

struct DrawContext;
struct Node;
union SDL_Event;

namespace YAML {
    struct Node;
}

// --- Factory macros ---
// To be included in every component's header.
#define COMPONENT_FACTORY_H(class)                                               \
class() {} \
class(YAML::Node p_data); \
static void instantiate(YAML::Node p_data, std::shared_ptr<Node> p_node); \
private:                                                                  \
    static bool registered;

// To be included in every component's implementation file, followed by
// { code initializing component using YAML node stored in p_data }
#define COMPONENT_FACTORY_IMPL(class, name) \
void class::instantiate(YAML::Node p_data, std::shared_ptr<Node> p_node) { \
    auto component = p_node->add_component<class>(p_data); \
    component->initialize(); \
} \
bool class::registered = Component::register_type(#name, &instantiate); \
class::class(YAML::Node p_data)
// ----------------------


struct Component {
    Node* node;

    static std::unordered_map<std::string, void(*)(YAML::Node, std::shared_ptr<Node>)> create_functions;

    virtual void initialize() {}
    virtual void update(double delta) {}
    virtual void process_input(SDL_Event& event) {}
    virtual void draw(const Mat4& p_transform, DrawContext& p_context) const {}
    virtual void cleanup() {}

    static bool register_type(const std::string p_name, void(*)(YAML::Node, std::shared_ptr<Node>));
};