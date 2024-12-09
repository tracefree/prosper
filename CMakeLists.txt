cmake_minimum_required(VERSION 3.31)
project(prosper VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_FLAGS "-g")

find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3-shared)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

add_subdirectory(third_party/fastgltf)

add_library(stb_image INTERFACE)

set(IMGUI_SOURCES
  third_party/imgui/imgui.cpp
  third_party/imgui/imgui_draw.cpp
  third_party/imgui/imgui_tables.cpp
  third_party/imgui/imgui_widgets.cpp
  third_party/imgui/backends/imgui_impl_sdl3.cpp
  third_party/imgui/backends/imgui_impl_vulkan.cpp
)

add_executable(prosper
    src/vma_usage.cpp
    src/main.cpp
    src/camera.cpp
    src/renderer.cpp
    src/vk_types.cpp
    src/vk_pipelines.cpp
    src/vk_shader_object.cpp
    src/vk_loader.cpp
    src/vk_descriptors.cpp
    src/stb_image_usage.cpp
    ${IMGUI_SOURCES}
)

set_property(
  TARGET prosper
  PROPERTY LINKER MOLD
)

target_include_directories(prosper PRIVATE
  src
  shaders/src/common
  third_party/imgui
  third_party/imgui/backends
)

target_include_directories(stb_image INTERFACE third_party/stb/)


target_compile_definitions(prosper PUBLIC
  VULKAN_HPP_NO_CONSTRUCTORS
  VULKAN_HPP_NO_EXCEPTIONS
  VULKAN_HPP_NO_NODISCARD_WARNINGS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VK_USE_PLATFORM_WAYLAND_KHR
  GLM_FORCE_DEPTH_ZERO_TO_ONE
)

include_directories(third_party/imgui/{*.h})


target_precompile_headers(prosper PUBLIC <optional> <vector> <string> <unordered_map> "src/vulkan_pch.h")
target_link_libraries(prosper PRIVATE SDL3::SDL3 Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator fastgltf::fastgltf stb_image)

