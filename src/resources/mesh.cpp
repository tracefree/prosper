#include <resources/mesh.h>
#include <rendering/renderer.h>

extern Renderer gRenderer;


template<>
void Resource<Mesh>::unload() {
    std::println("Unloading mesh: {}", guid.c_str());
    gRenderer.destroy_buffer(pointer->mesh_buffers.index_buffer);
    gRenderer.destroy_buffer(pointer->mesh_buffers.vertex_buffer);
    gRenderer.destroy_buffer(pointer->mesh_buffers.skinned_vertex_buffer);
    if (pointer->mesh_buffers.skinning_data_buffer.buffer != VK_NULL_HANDLE) {
        gRenderer.destroy_buffer(pointer->mesh_buffers.skinning_data_buffer);
    }
    if (pointer->mesh_buffers.joint_matrices_buffer.buffer != VK_NULL_HANDLE) {
        gRenderer.destroy_buffer(pointer->mesh_buffers.joint_matrices_buffer);
    }
};