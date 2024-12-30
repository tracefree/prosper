#include <components/mesh_instance.h>
#include <core/node.h>

void MeshInstance::draw(const Mat4& p_transform, DrawContext& p_context) {
    Mat4 node_matrix = p_transform * node->get_global_transform().get_matrix();

    for (auto& surface : mesh->surfaces) {
        RenderObject object {
            .index_count = surface.count,
            .first_index = surface.start_index,
            .index_buffer = mesh->mesh_buffers.index_buffer.buffer,
            .material = (surface.material).get(),
            .transform = node_matrix,
            .vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address,
            .skinned_vertex_buffer_address = mesh->mesh_buffers.skinned_vertex_buffer_address,
        };

        if (surface.material->pass_type == MaterialPass::Transparent) {
            p_context.transparent_surfaces.emplace_back(object);
        } else {
            p_context.opaque_surfaces.emplace_back(object);
        }   
    }
}

std::string MeshInstance::get_name() {
    return "MeshInstance";
}

MeshInstance::MeshInstance(Resource<Mesh> p_mesh) {
    mesh = p_mesh;
}