cmake_minimum_required(VERSION 3.21)
project(prosper VERSION 0.2 LANGUAGES CXX C)

# --- Options ---
option(PROSPER_PREFER_SYSTEM_LIBRARIES "Prefer system libraries for dependencies." ON)

# --- Dependencies ---
include(FetchContent)
set(CMAKE_CXX_FLAGS                " ${CMAKE_CXX_FLAGS_INIT} -std=c++17 -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG          " ${CMAKE_CXX_FLAGS_INIT} -g -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL     " ${CMAKE_CXX_FLAGS_INIT} -Os -DNDEBUG -s")
set(CMAKE_CXX_FLAGS_RELEASE        " ${CMAKE_CXX_FLAGS_INIT} -O1 -DNDEBUG -s -flto")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO " ${CMAKE_CXX_FLAGS_INIT} -O2 -g -DDEBUG")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
message(STATUS "Prosper: Checking dependencies.")

# - Vulkan
find_package(Vulkan REQUIRED)
find_package(VulkanUtilityLibraries REQUIRED)

# - SDL
function(fetch_sdl)
  message(STATUS "Prosper: 'SDL3': Fetching content.")
  FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG main
  )
  FetchContent_MakeAvailable(SDL3)
endfunction()

if(PROSPER_PREFER_SYSTEM_LIBRARIES)
  find_package(SDL3 CONFIG COMPONENTS SDL3-shared)
  if(${SDL3_FOUND})
    message(STATUS "Prosper: 'SDL3': Using system library.")
  else()
    fetch_sdl()
  endif()
else()
  fetch_sdl()
endif()

# - glm
function(fetch_glm)
  message(STATUS "Prosper: 'glm': Fetching content.")
  FetchContent_Declare(
    glm
    GIT_REPOSITORY	https://github.com/g-truc/glm.git
    GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
  )
  FetchContent_MakeAvailable(glm)
endfunction()

if(PROSPER_PREFER_SYSTEM_LIBRARIES)
  find_package(glm QUIET)
  if(${glm_FOUND})
    message(STATUS "Prosper: 'glm': Using system library.")
  else()
    fetch_glm()
  endif()
else()
  fetch_glm()
endif(PROSPER_PREFER_SYSTEM_LIBRARIES)

# - VulkanMemoryAllocator
function(fetch_vma)
  message(STATUS "Prosper: 'VulkanMemoryAllocator': Fetching content.")
  FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.1.0
  )
  FetchContent_MakeAvailable(VulkanMemoryAllocator)
endfunction()

if(PROSPER_PREFER_SYSTEM_LIBRARIES)
  find_package(VulkanMemoryAllocator QUIET)
  if(${VulkanMemoryAllocator_FOUND})
    message(STATUS "Prosper: 'VulkanMemoryAllocator': Using system library.")
  else()
    fetch_vma()
  endif()
else()
  fetch_vma()
endif()

# - yaml-cpp
function(fetch_yaml)
  message(STATUS "Prosper: 'yaml-cpp': Fetching content.")
  set(YAML_CPP_BUILD_TOOLS OFF)
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
  )
  FetchContent_MakeAvailable(yaml-cpp)
endfunction()

if(PROSPER_PREFER_SYSTEM_LIBRARIES)
  find_package(yaml-cpp QUIET)
  if(${yaml-cpp_FOUND})
    message(STATUS "Prosper: 'yaml-cpp': Using system library.")
  else()
    fetch_yaml()
  endif()
else()
  fetch_yaml()
endif()

# - fastgltf
message(STATUS "Prosper: 'fastgltf': Fetching content.")
FetchContent_Declare(
  fastgltf
  GIT_REPOSITORY https://github.com/spnda/fastgltf.git
  GIT_TAG v0.8.0
)
FetchContent_MakeAvailable(fastgltf)

# - Dear ImGui
message(STATUS "Prosper: 'Dear ImGui': Fetching content.")
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.6
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends/
)

# - STB
message(STATUS "Prosper: 'stb': Fetching content.")
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# Jolt Physics
message(STATUS "Prosper: 'JoltPhysics': Fetching content.")
FetchContent_Declare(
  JoltPhysics
  GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
  GIT_TAG v5.2.0
  SOURCE_SUBDIR Build
)
FetchContent_MakeAvailable(JoltPhysics)

# - KTX-Software
function(fetch_ktx)
  message(STATUS "Prosper: 'KTX-Software': Fetching content.")
  set(KTX_FEATURE_DOC OFF)
  set(KTX_FEATURE_GL_UPLOAD OFF)
  set(KTX_FEATURE_TESTS OFF)
  set(KTX_FEATURE_TOOLS OFF)
  set(KTX_FEATURE_TOOLS_CTS OFF)
  FetchContent_Declare(
    KTX-Software
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG main
  )
  FetchContent_MakeAvailable(KTX-Software)
endfunction()

if(PROSPER_PREFER_SYSTEM_LIBRARIES)
  find_package(KTX-Software QUIET)
  if(${KTX-Software_FOUND})
    message(STATUS "Prosper: 'KTX-Software': Using system library.")
  else()
    fetch_ktx()
  endif()
else()
  fetch_ktx()
endif()

# --- Main project ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_FLAGS         " ${CMAKE_CXX_FLAGS_INIT} -std=c++23 -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE " ${CMAKE_CXX_FLAGS_INIT} -std=c++23 -fPIC -O1 -DNDEBUG -s -flto")

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
    src/components/state_machine.cpp
    
    src/components/physics/rigid_body.cpp
    src/components/physics/static_body.cpp

    src/states/air.cpp
    src/states/dodge.cpp
    src/states/idle.cpp
    src/states/move.cpp
)

set_property(
  TARGET prosper
  PROPERTY LINKER MOLD
)

target_include_directories(prosper PUBLIC
  src
  shaders/src/common
  ${stb_SOURCE_DIR}
  ${ktx-software_SOURCE_DIR}/include/
  ${joltphysics_SOURCE_DIR}
)

target_compile_definitions(prosper PUBLIC
  VULKAN_HPP_NO_CONSTRUCTORS
  VULKAN_HPP_NO_EXCEPTIONS
  VULKAN_HPP_NO_NODISCARD_WARNINGS
  VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
  VK_USE_PLATFORM_WAYLAND_KHR
  GLM_FORCE_DEPTH_ZERO_TO_ONE
)

target_precompile_headers(prosper PUBLIC <optional> <vector> <string> <unordered_map> <print> <memory> "src/rendering/vulkan_pch.h")
target_link_libraries(imgui PUBLIC SDL3::SDL3 Vulkan::Vulkan)
target_link_libraries(prosper PUBLIC glm yaml-cpp::yaml-cpp fastgltf::fastgltf imgui ktx Jolt SDL3::SDL3 Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator stb)
