#!/usr/bin/env bash

/opt/shader-slang-bin/bin/slangc -target spirv -emit-spirv-directly -fvk-use-entrypoint-name -source-embed-style u32 -source-embed-name skybox_spv -o shaders/bin/skybox.h $(dirname "$0")/src/skybox.slang
/opt/shader-slang-bin/bin/slangc -target spirv -emit-spirv-directly -fvk-use-entrypoint-name -source-embed-style u32 -source-embed-name mesh_spv -o shaders/bin/mesh.h $(dirname "$0")/src/mesh.slang
/opt/shader-slang-bin/bin/slangc -target spirv -emit-spirv-directly -fvk-use-entrypoint-name -source-embed-style u32 -source-embed-name lighting_spv -o shaders/bin/lighting.h $(dirname "$0")/src/lighting.slang
/opt/shader-slang-bin/bin/slangc -target spirv -emit-spirv-directly -fvk-use-entrypoint-name -source-embed-style u32 -source-embed-name skinning_spv -o shaders/bin/skinning.h $(dirname "$0")/src/skinning.slang
