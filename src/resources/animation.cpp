#include <resources/animation.h>


std::vector<std::string> AnimationLibrary::get_animation_list() {
    std::vector<std::string> animation_list;
    animation_list.reserve(animations.size());
    for (auto& [name, _] : animations) {
        animation_list.push_back(name);
    }
    return animation_list;
}

std::vector<const char*> AnimationLibrary::get_animation_list_cstr() {
    std::vector<const char*> animation_list;
    animation_list.reserve(animations.size());
    for (auto& [name, _] : animations) {
        animation_list.push_back(name.c_str());
    }
    return animation_list;
}