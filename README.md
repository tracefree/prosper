# Prosper
C++ rendering engine using [Vulkan](https://www.vulkan.org/), the [Slang](https://shader-slang.com/) shader language, and [SDL3](https://wiki.libsdl.org/SDL3/FrontPage). (Work in progress.)

![prosper3](https://github.com/user-attachments/assets/f34fb9a6-0b2c-4d97-a27f-cafe68ebcb4a)

This project originally started as a minimal "Hello World" example to render a single triangle which I [moved to a new repository](https://github.com/tracefree/vulkan-triangle-sdl-slang) for anyone interested in a less complex setup using Slang and Vulkan. I will continue to develop the engine in this repository. It is not supposed to be a general purpose rendering engine - the primary goals of Prosper are to serve as a testing ground to me for experimenting with graphics programming techniques, and to be used for a video game I will be making. Therefore I will only implement features I am interested in and that are directly useful to the visual style I'm going for.

Based on [vkguide.dev](https://vkguide.dev), with the main differences being that I use Slang instead of GLSL and the VulkanHpp headers instead of the C API. Also very helpful resources were the [Vulkan Lecture Series](https://www.youtube.com/playlist?list=PLmIqTlJ6KsE1Jx5HV4sd2jOe3V1KMHHgn) by TU Wien and [vulkan-tutorial.com](https://vulkan-tutorial.com/).

## Current and planned features (non-exhaustive)
- [x] [Buffer device addresses](https://docs.vulkan.org/samples/latest/samples/extensions/buffer_device_address/README.html)
- [X] [Dynamic rendering](https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering/README.html) instead of render passes
- [x] [Shader objects](https://docs.vulkan.org/samples/latest/samples/extensions/shader_object/README.html) instead of pipelines
- [ ] Implement `VK_LAYER_KHRONOS_shader_object` to remove reliance on `VK_EXT_shader_object`
- [x] Normal mapping
- [ ] GPU driven rendering
- [ ] Deffered rendering
- [ ] Skeletal animation
- [ ] Integrating a physics engine
- [ ] HDR with tonemapping
- [ ] Bloom
- [ ] MSAA
- [ ] SSAO
- [ ] Either PBR or a shading model useful for stylized rendering
- [ ] Dynamic lighting
- [ ] Global illumination
- [ ] Anything else I come up with along the way

## Installation instructions
### Linux (and probably Mac and BSD)
You'll need to have installed the Vulkan SDK and SDL3 libraries on your system. To build with CMake run these commands in the terminal inside the project's directoy:

```
mkdir build
cd build
cmake ..
cmake --build .
```

The executable then gets created in `build/bin` if everything went right. 
Regarding the shaders, they are compiled to bytecode and included with `shaders/bin/` and their source code is in `shaders/src/`. If you want to modify it you'll have to compile it with [`slangc`](https://github.com/shader-slang/slang). The `shaders` directory contains a bash script with the compile commands I used, you will need to adjust path to the slangc binary to point to where it is installed on your system.

The project is currently hardcoded to load the [Sponza](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/Sponza) scene on startup but I do not include the model in this repository. In order for the program to run, download the folder from the link, create the directory `assets/models` in the project root folder and put the Sponza directory inside it.

### Windows
idk, you're on your own ¯\_(ツ)_/¯

## License
Libraries in the `third_party` directory include their respectve licenses. [fastgltf](https://github.com/spnda/fastgltf) and [Dear ImGui](https://github.com/ocornut/imgui) use the MIT license, and [stb_image.h](https://github.com/nothings/stb) lets you choose between public domain and MIT.

The code inside `src` currently is heavily based on the snippets provied by [vkguide.dev](https://github.com/vblanco20-1/vulkan-guide/tree/all-chapters-2) which is licensed under MIT. To the extent to which my substantial modifications are my own and where my code is original I license it under your choice of either public domain or the MIT license.

<p align="center">
  <img src="https://github.com/user-attachments/assets/63ba0ee4-e922-47bf-a14a-fc87a84f2947" alt="Prosper's logo, a rainbow colored hand made of pointy shapes doing the Vulkan greeting sign."/>
</p>
