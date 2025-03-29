# Prosper
C++ rendering/game engine using [Vulkan](https://www.vulkan.org/), the [Slang](https://shader-slang.com/) shader language, and [SDL3](https://wiki.libsdl.org/SDL3/FrontPage). Work in progress.

![prosper](https://github.com/user-attachments/assets/4e6cc111-e4b5-4894-8a68-e53faa229e4a)

This project originally started as a minimal "Hello World" example to render a single triangle which I [moved to a new repository](https://github.com/tracefree/vulkan-triangle-sdl-slang) for anyone interested in a less complex setup using Slang and Vulkan. I will continue to develop the engine in this repository. It is not supposed to be a general purpose engine - the primary goals of Prosper are to serve as a testing ground to me for experimenting with graphics programming techniques, and to be used for a video game I will be making. Therefore I will only implement features I am interested in and that are directly useful to the visual style I'm going for.

Initially based on the tutorial by [vkguide.dev](https://vkguide.dev), now with heavy modifications and extensions. Also very helpful resources were the [Vulkan Lecture Series](https://www.youtube.com/playlist?list=PLmIqTlJ6KsE1Jx5HV4sd2jOe3V1KMHHgn) by TU Wien and [vulkan-tutorial.com](https://vulkan-tutorial.com/).

## Current and planned features (non-exhaustive list)
Rendering:
- [x] [Buffer device addresses](https://docs.vulkan.org/samples/latest/samples/extensions/buffer_device_address/README.html)
- [X] [Dynamic rendering](https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering/README.html) instead of render passes
- [x] [Shader objects](https://docs.vulkan.org/samples/latest/samples/extensions/shader_object/README.html) instead of pipelines
- [ ] Implement `VK_LAYER_KHRONOS_shader_object` to remove reliance on `VK_EXT_shader_object`
- [x] Normal mapping
- [ ] GPU driven rendering
- [x] Deffered rendering
- [x] Skeletal animation
- [x] HDR with tonemapping
- [ ] Bloom
- [x] MSAA
- [ ] SSAO
- [ ] Either PBR or a shading model useful for stylized rendering
- [x] Directional and point lights
- [ ] Shadows
- [ ] Global illumination
- [x] Skybox
- [ ] etc.

General game engine features:
- [x] Basic character controller with third person camera
- [x] Make-shift scene format based on .yaml files 
- [x] Integrating the Jolt physics engine
- [x] Observer pattern with signals
- [x] Node / component system
- [ ] Modding support
- [ ] RPG gameplay systems
- [ ] etc.

Here's a short demonstration of the custom skeletal animation system:

[animation.webm](https://github.com/user-attachments/assets/63fabd7d-53f6-4e16-88f1-a4f0dca6ebe6)

## Installation instructions
### Linux (and probably Mac and BSD)
You'll need to have installed the Vulkan SDK, SDL3, and glm libraries on your system. To build with CMake, run these commands in the terminal inside the project's directoy:

```
cmake . -B build -G Ninja
cmake --build build
```

An example application using the engine library is provided in the `sample` directory. Build it by executing the same commands as above again within that directory, the executable `prosper_sample` will then be built in it. The sample project is currently set to automatically download the Sponza scene from the [glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets) repository, please refer to that page for license information for the assets. The camera can be controlled by looking around with the mouse and moving with the WASD keys, as well as E and Q for going up and down. The sample scene currently does not showcase the full capabilities of the engine - I will update it with an animated character and skybox once I decide on a better way to distribute assets.

Regarding the shaders, they are compiled to bytecode and included with `shaders/bin/` and their source code is in `shaders/src/`. If you want to modify it you'll have to compile it with [`slangc`](https://github.com/shader-slang/slang). The `shaders` directory contains a bash script with the compile commands I used, you will need to adjust path to the slangc binary to point to where it is installed on your system.

### Windows
idk, you're on your own ¯\_(ツ)_/¯

## Dependencies
Currently I use the following third party libraries, downloaded and built automatically via CMake:
- [Dear ImGui](https://github.com/ocornut/imgui)
- [fastgltf](https://github.com/spnda/fastgltf)
- [stb_image.h](https://github.com/nothings/stb)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp.git)
- [libktx](https://github.com/KhronosGroup/KTX-Software.git)
- [Jolt Physics](https://github.com/jrouwe/JoltPhysics.git)

## License
The Prosper engine source code is available under the [MIT license](LICENSE.md). Some of the Vulkan specific code is based on snippets from [vkguide.dev](https://github.com/vblanco20-1/vulkan-guide/tree/all-chapters-2) which is also licensed under MIT. For the dependencies, please refer to the respective repositories for license information.

<p align="center">
  <img src="https://github.com/user-attachments/assets/63ba0ee4-e922-47bf-a14a-fc87a84f2947" alt="Prosper's logo, a rainbow colored hand made of pointy shapes doing the Vulkan greeting sign."/>
</p>
