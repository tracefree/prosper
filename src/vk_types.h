#pragma once

#include <math.h>

#include <memory>
#include <chrono>
#include "vk_mem_alloc.h"

using namespace vk;

struct ShaderObject;

struct AllocatedImage {
    Image image;
    ImageView image_view;
    Format image_format;
    VmaAllocation allocation;
    Extent3D image_extent;
};

struct AllocatedBuffer {
    Buffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct Vertex {
    Vec3 position;
    float uv_x;
    Vec3 normal;
    float uv_y;
    Vec4 color;
    Vec4 tangent;
};

struct GPUMeshBuffers {
    AllocatedBuffer index_buffer;
    AllocatedBuffer vertex_buffer;
    DeviceAddress vertex_buffer_address;
};

struct GPUDrawPushConstants {
    Mat4 model_matrix;
    Mat4 view_projection;
    DeviceAddress vertex_buffer_address;
};

struct SceneLightsData {
    Mat4 view;
    Mat4 projection;
    Mat4 view_projection;
    Vec4 ambient_color;
    Vec4 sunlight_direction;
    Vec4 sunlight_color;
};

struct LightingPassPushConstants {
    uint32_t flags;
    float white_point;
};

enum class MaterialPass : uint8_t {
    MainColor,
    Transparent,
    Other,
};

struct MaterialPipeline {
    Pipeline pipeline;
    PipelineLayout layout;
};

struct MaterialInstance {
    ShaderObject* shader;
    MaterialPipeline* pipeline;
    DescriptorSet material_set;
    MaterialPass pass_type;
};

struct MeshSurface {
    uint32_t start_index;
    uint32_t count;
    std::shared_ptr<MaterialInstance> material;
};

struct MeshAsset {
    std::string name;
    std::vector<MeshSurface> surfaces;
    GPUMeshBuffers mesh_buffers;
};

struct RenderObject {
    uint32_t index_count;
    uint32_t first_index;
    Buffer index_buffer;

    MaterialInstance* material;

    Mat4 transform;
    DeviceAddress vertex_buffer_address;
};

struct DrawContext {
    std::vector<RenderObject> opaque_surfaces;
    std::vector<RenderObject> transparent_surfaces;
};

class IRenderable {
    virtual void draw(const Mat4& p_transform, DrawContext& p_context) = 0;
};

struct Node : public IRenderable {
    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    Mat4 transform;
    Mat4 global_transform;

    void refresh_transform(const Mat4& p_parent_transform);
    virtual void draw(const Mat4& p_transform, DrawContext& p_context);
};

struct MeshInstance : public Node {
    std::shared_ptr<MeshAsset> mesh;

    void draw(const Mat4& p_transform, DrawContext& p_context) override;
};

struct PerformanceStats {
    float frametime;
    uint32_t triangle_count;
    uint32_t drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
};