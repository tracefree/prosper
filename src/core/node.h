#pragma once
#include <math.h>
#include <core/component.h>
#include <type_traits>
#include <util.h>

union SDL_Event;

struct Node {
protected:
    Transform transform {};
    Transform global_transform {};
    // TODO: flag to only recalculate global transform when necessary

    std::unordered_map<const std::type_info*, Ref<Component>> component_table;

public:
    std::string name {"New node"};
    std::weak_ptr<Node> parent;
    std::vector<Ref<Node>> children;

    std::vector<Ref<Component>> components;
    bool active  { true };
    bool visible { true };

    Vec3 get_position() const;
    void set_position(Vec3 p_position);
    void set_position(float p_x, float p_y, float p_z);
    
    void move(Vec3 p_vector);
    void move(float p_x, float p_y, float p_z);

    float get_scale() const;
    void set_scale(float p_scale);
    void scale_by(float p_scale);

    glm::quat get_rotation() const;
    void set_rotation(glm::quat p_rotation);
    void rotate(Vec3 p_axis, float p_angle);

    Transform get_global_transform() const;

    void add_child(Ref<Node> p_child);

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Component, T>, void>
    add_component(Ref<T> p_component) {
        p_component->node = this;
        components.push_back(p_component);
        component_table[&typeid(T)] = p_component;
    }
    
    template<typename T, typename... Ts>
    std::enable_if_t<std::is_base_of_v<Component, T>, Ref<T>>
    add_component(Ts... arguments) {
        auto component = std::make_shared<T>(arguments...);
        component->node = this;
        components.push_back(component);

        component_table[&typeid(T)] = component;
        return component;
    }

    template<typename T>
    std::enable_if_t<std::is_base_of_v<Component, T>, Ref<T>>
    get_component() {
        return static_pointer_cast<T>(component_table[&typeid(T)]);
    }

    void refresh_transform();
    void refresh_transform(Transform p_parent_transform);

    void update(double delta);
    void process_input(SDL_Event& event);
    void draw(const Mat4& p_transform, DrawContext& p_context) const;
    void cleanup();

    static Ref<Node> create();
    static Ref<Node> create(std::string p_name);

    Node() {};
    Node(std::string p_name);
};