#include <components/skinned_mesh.h>


void SkinnedMesh::draw(const Mat4& p_transform, DrawContext& p_context) const {
    p_context.skinned_meshes.emplace_back(SkinningPushConstants {
        .number_vertices = (*mesh)->vertex_count,
        .input_vertex_buffer_address   = (*mesh)->mesh_buffers.vertex_buffer_address,
        .output_vertex_buffer_address  = (*mesh)->mesh_buffers.skinned_vertex_buffer_address,
        .skinning_data_buffer_address  = (*mesh)->mesh_buffers.skinning_data_buffer_address,
        .joint_matrices_buffer_address = (*mesh)->mesh_buffers.joint_matrices_buffer_address,
    });
}

void SkinnedMesh::cleanup() {
    mesh->unreference();
}