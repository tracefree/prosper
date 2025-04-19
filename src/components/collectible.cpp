#include <components/collectible.h>
#include <core/node.h>
#include <util.h>
#include <yaml.h>

extern Ref<Node> player;

int Collectible::collected_count = 0;
int Collectible::total_count = 0;

void Collectible::update(double delta) {
    if (!enabled || player == nullptr) {
        return;
    }

    float distance = glm::length(
        player->get_global_transform().position +
        Vec3(0.0, 1.0, 0.0) - 
        node->get_global_transform().position
    );
    
    float radius = node->get_scale() * 0.15f;
    if (distance < radius) {
        collected.emit(node);
        enabled = false;
        node->visible = false;
        Collectible::collected_count += 1;
    }
}

COMPONENT_FACTORY_IMPL(Collectible, collectible) {
    Collectible::total_count += 1;
}