#pragma once

#include <core/resource.h>
#include <rendering/types.h>


struct GPUMeshBuffers {
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    AllocatedBuffer skinned_vertex_buffer;
    AllocatedBuffer skinning_data_buffer;
    AllocatedBuffer joint_matrices_buffer;
    DeviceAddress vertex_buffer_address;
    DeviceAddress skinned_vertex_buffer_address;
    DeviceAddress skinning_data_buffer_address;
    DeviceAddress joint_matrices_buffer_address;
};

struct MeshSurface {
    uint32_t start_index;
    uint32_t count;
    std::shared_ptr<MaterialInstance> material;
};

struct Mesh {
    std::string name;
    uint32_t vertex_count {0};
    std::vector<MeshSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
};