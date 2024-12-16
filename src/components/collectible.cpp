#include <components/collectible.h>
#include <core/node.h>

extern std::shared_ptr<Node> player;

void Collectible::update(double delta) {
    if (!enabled) {
        return;
    }

    float distance = glm::length(
        player->get_global_transform().position +
        Vec3(0.0, 1.0, 0.0) - 
        node->get_global_transform().position
    );
    
    if (distance < 1.0f) {
        collected.emit(std::make_shared<Node>(*node));
        enabled = false;
    }
}

std::string Collectible::get_name() {
    return "Collectible";
}