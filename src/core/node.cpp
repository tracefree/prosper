#include <core/node.h>

void Node::refresh_transform() {
    refresh_transform(parent.lock() == nullptr ? Transform() : parent.lock()->global_transform);
}

void Node::refresh_transform(Transform p_parent_transform) {
    // TODO: Why can't p_parent_transform be const?
    global_transform = p_parent_transform * transform;
    
    for (auto child : children) {
        child->refresh_transform(global_transform);
    }
}

void Node::update(double delta) {
    for (auto& component : components) {
        component->update(delta);
    }
    for (auto& child : children) {
        child->update(delta);
    }
}

void Node::process_input(SDL_Event& event) {
    for (auto& component : components) {
        component->process_input(event);
    }
    for (auto& child : children) {
        child->process_input(event);
    }
}

void Node::draw(const Mat4& p_transform, DrawContext& p_context) {
    for (auto& component : components) {
        component->draw(p_transform, p_context);
    }
    for (auto child : children) {
        child->draw(p_transform, p_context);
    }
}

void Node::cleanup() {
    for (auto& component : components) {
        component->cleanup();
    }
    for (auto child : children) {
        child->cleanup();
    }
}

void Node::add_child(std::shared_ptr<Node> p_child) {
    children.push_back(p_child);
}

Node::Node(std::string p_name) {
    name = p_name;
}

Vec3 Node::get_position() {
    return transform.position;
}

void Node::set_position(Vec3 p_position) {
    transform.position = p_position;
    refresh_transform();
}

void Node::set_position(float p_x, float p_y, float p_z) {
    transform.position = Vec3(p_x, p_y, p_z);
    refresh_transform();
}

void Node::move(Vec3 p_vector) {
    transform.position += p_vector;
    refresh_transform();
}

float Node::get_scale() {
    return transform.scale;
}

void Node::set_scale(float p_scale) {
    transform.scale = p_scale;
    refresh_transform();
}

void Node::scale_by(float p_scale) {
    transform.scale *= p_scale;
    refresh_transform();
}

glm::quat Node::get_rotation() {
    return transform.rotation;
}

void Node::set_rotation(glm::quat p_rotation) {
    transform.rotation = p_rotation;
    refresh_transform();
}

Transform Node::get_global_transform() {
    return global_transform;
}

std::shared_ptr<Node> Node::create() {
    return std::make_shared<Node>();
}

std::shared_ptr<Node> Node::create(std::string p_name) {
    return std::make_shared<Node>(p_name);
}