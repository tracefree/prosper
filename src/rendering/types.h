#pragma once

#include <math.h>

#include <memory>
#include <chrono>
#include <unordered_map>
#include <vk_mem_alloc.h>
#include <ktxvulkan.h>
#include <SDL3/SDL_events.h>

using namespace vk;

struct ShaderObject;

struct AllocatedImage {
    std::optional<ktxVulkanTexture> ktx_texture;
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

struct SkinningData {
    uint32_t joint_ids[4];
    Vec4 weights;
};

struct JointData {
    uint32_t index;
    int parent_index;
    Vec3 position;
    glm::quat rotation;
    float scale;
    Mat4 inverse_bind_matrix;
};

struct GPUPointLight {
    Vec3 position;
    float intensity;
    Vec3 color;
    float _padding1;
};

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

struct GPUDrawPushConstants {
    Mat4 model_matrix;
    Mat4 view_projection;
    DeviceAddress vertex_buffer_address;
};

struct SkinningPushConstants {
    float time;
    uint32_t number_vertices;
    DeviceAddress input_vertex_buffer_address;
    DeviceAddress output_vertex_buffer_address;
    DeviceAddress skinning_data_buffer_address;
    DeviceAddress joint_matrices_buffer_address;
};

struct SceneLightsData {
    Mat4 view;
    Mat4 projection;
    Mat4 view_projection;
    Mat4 inverse_projection;
    Vec4 ambient_color;
    Vec4 sunlight_direction;
    Vec4 sunlight_color;
    GPUPointLight point_lights[8];
};

struct LightingPassPushConstants {
    uint32_t flags;
    float white_point;
    uint32_t viewport_width;
    uint32_t viewport_height;
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
    uint32_t vertex_count {0};
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
    DeviceAddress skinned_vertex_buffer_address;
};

struct DrawContext {
    std::vector<RenderObject> opaque_surfaces;
    std::vector<RenderObject> transparent_surfaces;
    std::vector<SkinningPushConstants> skinned_meshes;
};

struct PerformanceStats {
    float frametime;
    uint32_t triangle_count;
    uint32_t drawcall_count;
    float scene_update_time;
    float mesh_draw_time;
    float time_since_start {0.0f};
};