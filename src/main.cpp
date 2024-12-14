#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <string>
#include <stdlib.h>

#include "util.h"
#include "renderer.h"
#include "camera.h"
#include "render_flags.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

SDL_Window* gWindow{ nullptr };
Renderer gRenderer;
Camera gCamera {};

Uint64 previous_time {0};

PerformanceStats gStats {};

bool gValidationLayersEnabled { true };
vk::SampleCountFlagBits gSamples { vk::SampleCountFlagBits::e4 };

auto boot_time = std::chrono::system_clock::now();

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    char env[] = "SDL_VIDEODRIVER=wayland";
    putenv(env);
    
    // Initialize app
    SDL_SetAppMetadata("Prosper", "0.1", "Prosper");
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
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1920);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1080);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Prosper");
    gWindow = SDL_CreateWindowWithProperties(window_props);
    if (gWindow == nullptr) {
        print("Window could not be created! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Surface* icon = SDL_LoadBMP("../../icon.bmp");
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

    gRenderer.loaded_scenes["scene"] = *load_gltf_scene(&gRenderer, "../../assets/models/Sponza/glTF/Sponza.gltf");

    SDL_ShowWindow(gWindow);
    SDL_SetWindowRelativeMouseMode(gWindow, true);
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate) {

    auto start_time = std::chrono::system_clock::now();

    gCamera.update(gStats.frametime / 1000.0f);

    if (gRenderer.resize_requested || true) {
        gRenderer.recreate_swapchain();
    }

    // - ImGui --
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    if (ImGui::Begin("Configure")) {
        ComputeEffect& selected = gRenderer.background_effects[gRenderer.current_background_effect];
        ImGui::CheckboxFlags("Show normals",    &gRenderer.flags, RENDER_FLAG_BIT_SHOW_NORMALS);
        ImGui::CheckboxFlags("Show metallic",   &gRenderer.flags, RENDER_FLAG_BIT_SHOW_METAL);
        ImGui::CheckboxFlags("Show roughness",  &gRenderer.flags, RENDER_FLAG_BIT_SHOW_ROUGHNESS);
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
            gCamera.controls_enabled = false;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        SDL_SetWindowRelativeMouseMode(gWindow, true);
        gCamera.controls_enabled = true;
    }

    gCamera.process_SDL_Event(*event);
    

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    gRenderer.cleanup();
}
