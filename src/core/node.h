#pragma once
#include <math.h>
#include <unordered_map>
#include <memory>
#include <SDL3/SDL_events.h>
#include <string>
#include <core/component.h>
#include <type_traits>

struct Node {
protected:
    Transform transform {};
    Transform global_transform {};
    // TODO: flag to only recalculate global transform when necessary

    std::unordered_map<const std::type_info*, std::shared_ptr<Component>> component_table;

public:
    std::string name {"New node"};
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    std::vector<std::shared_ptr<Component>> components;
    bool active  { true };
    bool visible { true };

    Vec3 get_position();
    void set_position(Vec3 p_position);
    void set_position(float p_x, float p_y, float p_z);
    
    void move(Vec3 p_vector);
    void move(float p_x, float p_y, float p_z);

    float get_scale();
    void set_scale(float p_scale);
    void scale_by(float p_scale);

    glm::quat get_rotation();
    void set_rotation(glm::quat p_rotation);

    Transform get_global_transform();

    void add_child(std::shared_ptr<Node> p_child);

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Component, T>, void>
    add_component(std::shared_ptr<T> p_component) {
        p_component->node = this;
        components.push_back(p_component);
        component_table[&typeid(T)] = p_component;
    }
    
    template<typename T>
    std::enable_if_t<std::is_base_of_v<Component, T>, std::shared_ptr<T>>
    add_component() {
        auto component = std::make_shared<T>();
        component->node = this;
        components.push_back(component);

        component_table[&typeid(T)] = component;
        return component;
    }

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Component, T>, std::shared_ptr<T>>
    get_component() {
        return static_pointer_cast<T>(component_table[&typeid(T)]);
    }

    void refresh_transform();
    void refresh_transform(Transform p_parent_transform);

    void update(double delta);
    void process_input(SDL_Event& event);
    void draw(const Mat4& p_transform, DrawContext& p_context);
    void cleanup();

    static std::shared_ptr<Node> create();
    static std::shared_ptr<Node> create(std::string p_name);

    Node() {};
    Node(std::string p_name);
};