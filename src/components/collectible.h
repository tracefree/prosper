#pragma once

#include <core/component.h>
#include <core/signal.h>

struct Collectible : Component {
    Signal<std::shared_ptr<Node>> collected {};
    bool enabled { true };

    void update(double delta) override;
    static std::string get_name();
};