#include <stdlib.h>
#include <SDL3/SDL.h>
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
#include <core/resource.h>
#include <core/resource_manager.h>
#include <core/scene_graph.h>

#include <components/animation_player.h>
#include <components/bone.h>
#include <components/camera.h>
#include <components/character_controller.h>
#include <components/collectible.h>
#include <components/skeleton.h>
#include <components/physics/rigid_body.h>
#include <components/physics/static_body.h>

#include <resources/scene.h>

#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <thread>

SDL_Window* gWindow{ nullptr };
Renderer gRenderer;
PerformanceStats gStats {};
SceneGraph scene;
vk::SampleCountFlagBits gSamples { vk::SampleCountFlagBits::e4 };
auto boot_time = std::chrono::system_clock::now();

Ref<Node> player;
std::thread physics_worker;
static bool engine_running { false };

namespace prosper {
    double accumulated_time = 0;
    constexpr double fixed_timestep = 1.0 / 60.0;

    bool process_event(SDL_Event* event) {
        ImGui_ImplSDL3_ProcessEvent(event);
        if (ImGui::GetIO().WantCaptureMouse) {
            return true;
        }

        if (event->type == SDL_EVENT_QUIT) {
            return false;
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

        return true;
    }

    bool iterate() {
        static bool first = true;
        if (first) {
            first = false;
            Physics::physics_system.OptimizeBroadPhase();
        }
        
        auto start_time = std::chrono::system_clock::now();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (!process_event(&event)) {
                return false;
            }
        }

        scene.update(gStats.frametime / 1000.0f);

        accumulated_time += (gStats.frametime / 1000.0f);
        while (accumulated_time >= fixed_timestep) {
            Physics::update(fixed_timestep);
            accumulated_time -= fixed_timestep;
        }
        
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
        //    ImGui::CheckboxFlags("Show roughness",  &gRenderer.flags, RENDER_FLAG_BIT_SHOW_ROUGHNESS);
            ImGui::CheckboxFlags("Show complexity", &gRenderer.flags, RENDER_FLAG_BIT_SHOW_COMPLEX_PIXELS);
            ImGui::SliderFloat("White point", &gRenderer.white_point, 0.1f, 10.0f);
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

        if (ImGui::Begin("Controls")) {
            ImGui::Text("Move: WASD");
            ImGui::Text("Look around: Mouse");
            ImGui::Text("Jump: Space");
            ImGui::Text("Sprint: Hold left Shift");
      //    ImGui::Text("Roll: Left Alt");
            ImGui::Text("Toggle cursor: Escape");
            ImGui::Text("Exit: Alt+F4");
        }
        ImGui::End();

        if (Collectible::total_count > 0) {
            if (ImGui::Begin("Collectibles")) {
                ImGui::Text("Krapfens collected: %d / %d", Collectible::collected_count, Collectible::total_count);
                if (Collectible::collected_count >= Collectible::total_count) {
                    ImGui::Text("You win!");
                }
            }
            ImGui::End();
        }

        ImGui::Render();
        // ----------

        gRenderer.draw();

        auto end_time = std::chrono::system_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        gStats.frametime = elapsed.count() / 1000.0f;

        auto total_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - boot_time);
        gStats.time_since_start = total_elapsed.count() / 1000000.0f;
        
        return true;
    }

    void quit() {
        engine_running = false;
      //  physics_worker.join();
        gRenderer.device.waitIdle();
        scene.cleanup();
        gRenderer.cleanup();
        Physics::cleanup();
    }

    void physics_iterate() {
        while (engine_running) {
            Physics::update(fixed_timestep);
            std::this_thread::sleep_for(std::chrono::microseconds(16667));
            std::println("physs");
        }
    }

    bool initialize() {
        char env[] = "SDL_VIDEODRIVER=wayland";
        putenv(env);
        
        // Initialize app
        SDL_SetAppMetadata("Prosper", "0.2", "Prosper");
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            SDL_Log("SDL could not initialize! SDL error: %s\n", SDL_GetError());
            return false;
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
            return false;
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

#define FLOOR 0
#if FLOOR
        // Floor
        auto floor = Node::create("Floor");
        scene.root->add_child(floor);
        auto static_body = floor->add_component<StaticBody>();
        floor->set_position(0.0, -1.0, 0.0);
        static_body->shape = new JPH::BoxShape(JPH::Vec3(100.0, 1.0, 100.0));
        static_body->initialize();
#endif
        
        scene.camera = Node::create("Camera");
        scene.camera->set_position(0.0f, 1.4f, 0.0f);
        scene.root->add_child(scene.camera);
        auto camera = scene.camera->add_component<Camera>();
        camera->initialize();

        engine_running = true;
        // TODO: Figure out thread safety
        //physics_worker = std::thread(physics_iterate);

        SDL_ShowWindow(gWindow);
        SDL_SetWindowRelativeMouseMode(gWindow, true);
        return true;
    }
}
