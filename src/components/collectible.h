#pragma once

#include <core/component.h>
#include <core/signal.h>


struct Collectible : Component {
    Signal<Node*> collected {};
    bool enabled { true };
    
    static int collected_count;
    static int total_count;

    void update(double delta) override;

    COMPONENT_FACTORY_H(Collectible)
};
