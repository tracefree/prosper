#include "vk_types.h"


void Node::refresh_transform(const Mat4& p_parent_transform) {
    global_transform = p_parent_transform * transform;
    for (auto child : children) {
        child->refresh_transform(global_transform);
    }
}

void Node::draw(const Mat4& p_transform, DrawContext& p_context) {
    for (auto child : children) {
        child->draw(p_transform, p_context);
    }
}

void MeshInstance::draw(const Mat4& p_transform, DrawContext& p_context) {
    Mat4 node_matrix = p_transform * global_transform;
    
    for (auto& surface : mesh->surfaces) {
        RenderObject object {
            .index_count = surface.count,
            .first_index = surface.start_index,
            .index_buffer = mesh->mesh_buffers.index_buffer.buffer,
            .material = (surface.material).get(),
            .transform = node_matrix,
            .vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address,
        };

        if (surface.material->pass_type == MaterialPass::Transparent) {
            p_context.transparent_surfaces.emplace_back(object);
        } else {
            p_context.opaque_surfaces.emplace_back(object);
        }   
    }

    Node::draw(p_transform, p_context);
}