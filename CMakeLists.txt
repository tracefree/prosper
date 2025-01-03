cmake_minimum_required(VERSION 3.31)
project(prosper VERSION 0.2 LANGUAGES CXX)

# --- Dependencies ---
#set(CMAKE_CXX_FLAGS "-O2 -flto -fno-fat-lto-objects")
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
find_package(Vulkan REQUIRED)
include(FetchContent)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS_INIT} -std=c++20 -fPIC")

# - VulkanMemoryAllocator
FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

# - yaml-cpp
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.7.0
)
FetchContent_MakeAvailable(yaml-cpp)

# - fastgltf
FetchContent_Declare(
  fastgltf
  GIT_REPOSITORY https://github.com/spnda/fastgltf.git
  GIT_TAG v0.8.0
)
FetchContent_MakeAvailable(fastgltf)

# - Dear ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.6
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
  build/_deps/imgui-src/imgui.cpp
  build/_deps/imgui-src/imgui_draw.cpp
  build/_deps/imgui-src/imgui_tables.cpp
  build/_deps/imgui-src/imgui_widgets.cpp
  build/_deps/imgui-src/backends/imgui_impl_sdl3.cpp
  build/_deps/imgui-src/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
  build/_deps/imgui-src/
  build/_deps/imgui-src/backends/
)

# - STB
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE build/_deps/stb_src/)

# - KTX-Software
set(KTX_FEATURE_DOC OFF)
set(KTX_FEATURE_GL_UPLOAD OFF)
set(KTX_FEATURE_TOOLS OFF)
FetchContent_Declare(
  KTX-Software
  GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
  GIT_TAG main
)
FetchContent_MakeAvailable(KTX-Software)

# Jolt Physics
FetchContent_Declare(
  JoltPhysics
  GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
  GIT_TAG v5.2.0
  SOURCE_SUBDIR Build
)
FetchContent_MakeAvailable(JoltPhysics)


# --- Main project ---
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_FLAGS                " ${CMAKE_CXX_FLAGS_INIT} -std=c++23 -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG          "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DDEBUG")

add_library(prosper SHARED
    src/prosper.cpp
    src/input.cpp
    src/math.cpp
    src/physics.cpp
    src/loader.cpp

    src/core/node.cpp
    src/core/component.cpp
    src/core/scene_graph.cpp

    src/rendering/descriptors.cpp
    src/rendering/renderer.cpp
    src/rendering/shader_object.cpp
    src/rendering/stb_image_usage.cpp
    src/rendering/vma_usage.cpp
    
    src/resources/animation.cpp
    src/resources/mesh.cpp
    src/resources/scene.cpp
    src/resources/texture.cpp

    src/components/animation_player.cpp
    src/components/bone.cpp
    src/components/camera.cpp
    src/components/character_controller.cpp
    src/components/collectible.cpp
    src/components/mesh_instance.cpp
    src/components/model_data.cpp
    src/components/point_light.cpp
    src/components/skeleton.cpp
    src/components/skinned_mesh.cpp
    
    src/components/physics/rigid_body.cpp
    src/components/physics/static_body.cpp
)

set_property(
  TARGET prosper
  PROPERTY LINKER MOLD
)

target_include_directories(prosper PUBLIC
  src
  shaders/src/common
  build/_deps/stb-src/
  build/_deps/ktx-software-src/include/
  build/_deps/joltphysics-src/
)

target_compile_definitions(prosper PUBLIC
  VULKAN_HPP_NO_CONSTRUCTORS
  VULKAN_HPP_NO_EXCEPTIONS
  VULKAN_HPP_NO_NODISCARD_WARNINGS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VK_USE_PLATFORM_WAYLAND_KHR
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  KHRONOS_STATIC # Needed to statically link KTX-Software
)

target_precompile_headers(prosper PUBLIC <optional> <vector> <string> <unordered_map> <print> <memory> "src/rendering/vulkan_pch.h")
target_link_libraries(imgui PUBLIC SDL3::SDL3 Vulkan::Vulkan)
target_link_libraries(prosper PUBLIC yaml-cpp::yaml-cpp fastgltf::fastgltf imgui ${CMAKE_SOURCE_DIR}/build/_deps/ktx-software-build/libktx.a Jolt SDL3::SDL3 Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator stb)