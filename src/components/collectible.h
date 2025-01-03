#pragma once

#include <core/component.h>
#include <core/signal.h>


struct Collectible : Component {
    Signal<Node*> collected {};
    bool enabled { true };

    void update(double delta) override;

    COMPONENT_FACTORY_H(Collectible)
};