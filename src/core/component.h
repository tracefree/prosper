#pragma once
#include <SDL3/SDL_events.h>
#include <math.h>
#include <rendering/types.h>

struct Node;

struct Component {
    Node* node;

    static std::string get_name();
    virtual void update(double delta) {};
    virtual void process_input(SDL_Event& event) {};
    virtual void draw(const Mat4& p_transform, DrawContext& p_context) {}
    virtual void cleanup() {};
};