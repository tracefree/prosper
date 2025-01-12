#include <components/collectible.h>
#include <core/node.h>
#include <util.h>
#include <yaml.h>

extern Ref<Node> player;

void Collectible::update(double delta) {
    if (!enabled || player == nullptr) {
        return;
    }

    float distance = glm::length(
        player->get_global_transform().position +
        Vec3(0.0, 1.0, 0.0) - 
        node->get_global_transform().position
    );
    
    if (distance < 1.0f) {
        collected.emit(node);
        enabled = false;
        node->visible = false;
    }
}

COMPONENT_FACTORY_IMPL(Collectible, collectible) {}