#include <prosper.h>
#include <core/node.h>
#include <core/scene_graph.h>
#include <resources/scene.h>
#include <core/resource.h>
#include <core/resource_manager.h>

extern SceneGraph scene;

int main() {
    if (!prosper::initialize()) {
        return 1;
    }

    auto level_scene = ResourceManager::load<Scene>("data/level.yaml");
    auto level = (*level_scene)->instantiate();
    scene.root->add_child(level);

    bool running = true;
    while (running) {
        running = prosper::iterate();
    }
    
    prosper::quit();
    return 0;
}