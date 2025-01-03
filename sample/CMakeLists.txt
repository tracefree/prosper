cmake_minimum_required(VERSION 3.31)
project(prosper_sample VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_FLAGS                " ${CMAKE_CXX_FLAGS_INIT} -std=c++23")
set(CMAKE_CXX_FLAGS_DEBUG          "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG -flto -fno-fat-lto-objects")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DDEBUG")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

include(FetchContent)

# - Sponza asset download
FetchContent_Declare(
    Sponza
    GIT_REPOSITORY https://github.com/KhronosGroup/glTF-Sample-Assets.git
    GIT_TAG main
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/assets
)
FetchContent_MakeAvailable(Sponza)

add_executable(prosper_sample
    src/main.cpp
)

set_property(
    TARGET prosper_sample
    PROPERTY LINKER MOLD
)
target_precompile_headers(prosper_sample PRIVATE <optional> <vector> <string> <unordered_map> <print> <memory>)
target_include_directories(prosper_sample PRIVATE ${CMAKE_SOURCE_DIR}/../src)

target_link_libraries(prosper_sample PRIVATE ${CMAKE_SOURCE_DIR}/../build/libprosper.so)