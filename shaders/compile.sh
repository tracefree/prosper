#!/usr/bin/env bash

/opt/shader-slang-bin/bin/slangc -target spirv -emit-spirv-directly -fvk-use-entrypoint-name -source-embed-style u32 -source-embed-name mesh_spv -o shaders/bin/mesh.h $(dirname "$0")/src/mesh.slang
/opt/shader-slang-bin/bin/slangc -target spirv -emit-spirv-directly -fvk-use-entrypoint-name -source-embed-style u32 -source-embed-name gradient_spv -o shaders/bin/gradient.h $(dirname "$0")/src/gradient.slang
