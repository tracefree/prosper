#include <components/mesh_instance.h>
#include <core/node.h>
#include <rendering/types.h>


void MeshInstance::draw(const Mat4& p_transform, DrawContext& p_context) const {
    Mat4 node_matrix = p_transform * node->get_global_transform().get_matrix();
    
    for (auto& surface : (*mesh)->surfaces) {
        RenderObject object {
            .index_count = surface.count,
            .first_index = surface.start_index,
            .index_buffer = (*mesh)->mesh_buffers.index_buffer.buffer,
            .material = (surface.material).get(),
            .transform = node_matrix,
            .vertex_buffer_address = (*mesh)->mesh_buffers.vertex_buffer_address,
            .skinned_vertex_buffer_address = (*mesh)->mesh_buffers.skinned_vertex_buffer_address,
        };

        if (surface.material->pass_type == MaterialPass::Transparent) {
            p_context.transparent_surfaces.emplace_back(object);
        } else {
            p_context.opaque_surfaces.emplace_back(object);
        }
    }
}