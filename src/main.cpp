#include <stdlib.h>
#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl3.h>
#include <print>

#include <util.h>
#include <input.h>
#include <physics.h>
#include <rendering/renderer.h>
#include <render_flags.h>

#include <core/node.h>
#include <core/scene_graph.h>

#include <components/animation_player.h>
#include <components/bone.h>
#include <components/camera.h>
#include <components/character_controller.h>
#include <components/collectible.h>
#include <components/physics/rigid_body.h>
#include <components/physics/static_body.h>

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

SDL_Window* gWindow{ nullptr };
Renderer gRenderer;
PerformanceStats gStats {};
SceneGraph scene;
vk::SampleCountFlagBits gSamples { vk::SampleCountFlagBits::e4 };
auto boot_time = std::chrono::system_clock::now();
bool gValidationLayersEnabled { true  };

std::shared_ptr<Bone> target_bone;
std::shared_ptr<Skeleton> target_skeleton;
std::shared_ptr<AnimationPlayer> animation_player;
std::shared_ptr<Node> player;

// Just a test function, to be removed
void on_krapfen_collected(std::shared_ptr<Node> p_node) {
    std:print("%s collected", p_node->name.c_str());
    p_node->visible = false; // TODO: Why doesn't this work?
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    char env[] = "SDL_VIDEODRIVER=wayland";
    putenv(env);
    
    // Initialize app
    SDL_SetAppMetadata("Prosper", "0.2", "Prosper");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL could not initialize! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Load Vulkan driver
    if(!SDL_Vulkan_LoadLibrary(nullptr)) {
        SDL_Log("Could not load Vulkan library! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create window
    SDL_PropertiesID window_props {SDL_CreateProperties()};
    SDL_SetNumberProperty(window_props,  SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1920);
    SDL_SetNumberProperty(window_props,  SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1080);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    SDL_SetStringProperty(window_props,  SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Prosper");
    gWindow = SDL_CreateWindowWithProperties(window_props);
    if (gWindow == nullptr) {
        print("Window could not be created! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Surface* icon = SDL_LoadBMP("icon.bmp");
    if(icon == nullptr) {
        print("Could not load icon! SDL Error: %s", SDL_GetError());
    } else {
        SDL_SetWindowIcon(gWindow, icon);
    }

    // Initialize Vulkan
    uint32_t sdl_extension_count;
    const char* const* sdl_extension_names = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    std::vector<const char*> extensions;
    for (int i = 0; i < sdl_extension_count; ++i) {
        extensions.emplace_back(sdl_extension_names[i]);
    }

    gRenderer.initialize(extensions.size(), extensions.data(), gWindow, 1920, 1080);

    // Initialize systems
    Input::initialize();
    Physics::initialize();

    // Initialize scene
    scene.root = Node::create("root");
    
    auto level = load_scene("level.yaml");
    scene.root->add_child(level);
   // level->set_scale(1.3f);

    auto krapfen = load_scene("krapfen.yaml");
    krapfen->set_position(3.0f, 2.0f, 0.0f);
    krapfen->scale_by(5.0f);
    auto rigid_body = krapfen->add_component<RigidBody>();
    rigid_body->shape = new JPH::SphereShape(0.5f);
    rigid_body->initialize();
    
    auto collectible = krapfen->add_component<Collectible>();
    collectible->collected.connect(on_krapfen_collected);
    scene.root->add_child(krapfen);

    auto krapfen2 = load_scene("krapfen.yaml");
    krapfen2->set_position(2.7f, 5.0f, 0.0f);
    
    auto rigid_body2 = krapfen2->add_component<RigidBody>();
    rigid_body2->shape = new JPH::BoxShape(JPH::Vec3(0.1, 0.1, 0.1));
    krapfen->set_position(3.0f, 5.0f, 0.0f);
    rigid_body2->initialize();
    scene.root->add_child(krapfen2);

    // Floor
    auto floor = Node::create("Floor");
    level->add_child(floor);
    auto static_body = floor->add_component<StaticBody>();

# define MESH_SHAPE 0
#if MESH_SHAPE
    JPH::TriangleList triangles;
    auto [vertices, indices] = load_triangles("assets/models/Sponza/glTF/Sponza.gltf");
    triangles.reserve(indices.size() * 3);
    for (uint triangle_id = 0; triangle_id + 3 < indices.size(); triangle_id += 3) {
        JPH::Vec3 v1(vertices[indices[triangle_id]].x,     vertices[indices[triangle_id]].y,     vertices[indices[triangle_id]].z);
        JPH::Vec3 v2(vertices[indices[triangle_id + 1]].x, vertices[indices[triangle_id + 1]].y, vertices[indices[triangle_id + 1]].z);
        JPH::Vec3 v3(vertices[indices[triangle_id + 2]].x, vertices[indices[triangle_id + 2]].y, vertices[indices[triangle_id + 2]].z);
        triangles.emplace_back(JPH::Triangle(v1, v2, v3, 0));
    }
    JPH::PhysicsMaterialList mats;
    mats.push_back(JPH::PhysicsMaterial::sDefault);
    auto mesh_settings = new JPH::MeshShapeSettings(triangles, mats);
    static_body->shape = mesh_settings->Create().Get();
#else
    floor->set_position(0.0, -1.0, 0.0);
    static_body->shape = new JPH::BoxShape(JPH::Vec3(100.0, 1.0, 100.0));
#endif
    static_body->initialize();

    auto camera_node = Node::create("Camera");
    camera_node->set_position(0.0f, 1.4f, 0.0f);
    auto camera = camera_node->add_component<Camera>();
    
    
    scene.camera = camera_node;
    scene.root->add_child(camera_node);

#define THIRD_PERSON_CHARACTER 1
#if THIRD_PERSON_CHARACTER
    player = load_scene("player.yaml");
    scene.root->add_child(player);
    auto character_controller = player->add_component<CharacterController>();
    character_controller->initialize();

    camera->body_to_exclude = character_controller->character->GetInnerBodyID();
    scene.camera->get_component<Camera>()->follow_target = player;
    scene.camera->get_component<Camera>()->yaw = M_PI_2;

    player->add_component<AnimationPlayer>(animation_player);
    animation_player->skeleton->root_motion_index = 46;
#endif

    camera->initialize();

    SDL_ShowWindow(gWindow);
    SDL_SetWindowRelativeMouseMode(gWindow, true);
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate) {
    auto start_time = std::chrono::system_clock::now();

    scene.update(gStats.frametime / 1000.0f);

    Physics::update();
   
    if (gRenderer.resize_requested || true) {
        gRenderer.recreate_swapchain();
    }

    // - ImGui --
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    if (ImGui::Begin("Configure")) {
        ImGui::CheckboxFlags("Show normals",    &gRenderer.flags, RENDER_FLAG_BIT_SHOW_NORMALS);
        ImGui::CheckboxFlags("Show metallic",   &gRenderer.flags, RENDER_FLAG_BIT_SHOW_METAL);
        ImGui::CheckboxFlags("Show roughness",  &gRenderer.flags, RENDER_FLAG_BIT_SHOW_ROUGHNESS);
        ImGui::CheckboxFlags("Show complexity", &gRenderer.flags, RENDER_FLAG_BIT_SHOW_COMPLEX_PIXELS);
        ImGui::SliderFloat("White point", &gRenderer.white_point, 0.1f, 10.0f);
    }
    ImGui::End();

    if (ImGui::Begin("Skeleton")) {
        static float head_size = 1.0f;
        if (ImGui::SliderFloat("Head size", &head_size, 0.1f, 5.0f)) {
            target_bone->node->set_scale(head_size);
        }

        static int current_item = 0;
        std::vector<const char*> animation_list = animation_player->library.get_animation_list_cstr();
        if (ImGui::Combo("Pose", &current_item, animation_list.data(), animation_list.size(), 50)) {
            animation_player->play(animation_list[current_item]);
        }
    }
    ImGui::End();

    if (ImGui::Begin("Stats")) {
        ImGui::Text("FPS:         %f", std::floor(1000.0f / gStats.frametime));
        ImGui::Text("Frametime:   %f ms", gStats.frametime);
        ImGui::Text("Draw time:   %f ms", gStats.mesh_draw_time);
        ImGui::Text("Update time: %f ms", gStats.scene_update_time);
        ImGui::Text("Triangles:   %i", gStats.triangle_count);
        ImGui::Text("Draw calls:  %i", gStats.drawcall_count);
    }
    ImGui::End();

    ImGui::Render();
    // ----------

    gRenderer.draw();

    auto end_time = std::chrono::system_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    gStats.frametime = elapsed.count() / 1000.0f;

    auto total_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - boot_time);
    gStats.time_since_start = total_elapsed.count() / 1000000.0f;
    
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appsatte, SDL_Event *event) {

    ImGui_ImplSDL3_ProcessEvent(event);
    if (ImGui::GetIO().WantCaptureMouse) {
        return SDL_APP_CONTINUE;
    }

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        gRenderer.recreate_swapchain();
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
            SDL_SetWindowRelativeMouseMode(gWindow, false);
            scene.camera->get_component<Camera>()->controls_enabled = false;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        SDL_SetWindowRelativeMouseMode(gWindow, true);
        scene.camera->get_component<Camera>()->controls_enabled = true;
    }

    Input::process_event(*event);
    scene.root->process_input(*event);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    gRenderer.device.waitIdle();
    scene.cleanup();
    gRenderer.cleanup();
    Physics::cleanup();
}