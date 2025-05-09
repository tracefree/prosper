#pragma once

#include <SDL3/SDL_vulkan.h>
#include <ktxvulkan.h>
#include <deque>
#include <functional>
#include <stddef.h>
#include <memory>
#include <vk_mem_alloc.h>

#include <loader.h>
#include <math.h>
#include <rendering/types.h>
#include <rendering/descriptors.h>
#include <rendering/shader_object.h>
#include <resources/mesh.h>
#include <resources/texture.h>

const int MAX_FRAMES_IN_FLIGHT = 4;

using namespace vk;

struct DeletionQueue {
private:
        std::deque<std::function<void()>> deletors;
public:
    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        for (auto iterator = deletors.rbegin(); iterator != deletors.rend(); iterator++) {
            (*iterator)();
        }

        deletors.clear();
    }
};

struct FrameData {
    CommandPool command_pool;
    CommandBuffer command_buffer;
    Semaphore swapchain_semaphore;
    Semaphore render_semaphore;
    Fence render_fence;
    DeletionQueue deletion_queue;
    DescriptorAllocatorGrowable descriptors;
    GPUDrawPushConstants push_constants;
};

struct ComputePushConstants {
    Vec4 data1;
    Vec4 data2;
    Vec4 _data3;
    Vec4 _data4;
};

struct MaterialMetallicRoughness {
    ShaderObject opaque_shader;
    ShaderObject transparent_shader;

    DescriptorSetLayout material_layout;

    struct MaterialConstants {
        Vec4 albedo_factors;
        Vec4 metal_roughness_factors;
        Vec4 extra[14];
    };

    struct MaterialResources {
        Ref<Resource<Texture>> albedo_texture;
        vk::Sampler albedo_sampler;

        Ref<Resource<Texture>> normal_texture;
        vk::Sampler normal_sampler;

        Ref<Resource<Texture>> metal_roughness_texture;
        vk::Sampler metal_roughness_sampler;

        vk::Buffer data_buffer;
        uint32_t data_buffer_offset;
    };

    DescriptorWriter writer;

    void build_shaders(Renderer* p_renderer);
    void clear_resources(Device p_device);

    MaterialInstance write_material(Device p_device, MaterialPass p_pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptor_allocatpr);
};


class Renderer {

public:
    SwapchainKHR swapchain;
    Format swapchain_image_format;
    std::vector<Image> swapchain_images;
    std::vector<ImageView> swapchain_image_views;
    SurfaceCapabilitiesKHR surface_capabilities;

    FrameData frames[MAX_FRAMES_IN_FLIGHT];
    
    Queue graphics_queue;
    uint32_t graphics_queue_index;
    
    uint32_t current_frame {0};
    std::vector<CommandBuffer> command_buffers;
    Extent2D viewport_size;
    DeletionQueue deletion_queue;
    VmaAllocator allocator;

    DescriptorAllocatorGrowable global_descriptor_allocator;
    DescriptorSet draw_image_descriptors;
    DescriptorSetLayout draw_image_descriptor_layout;
    DescriptorSetLayout storage_image_descriptor_layout;
    DescriptorSetLayout skybox_descriptor_layout;

    PipelineLayout skybox_layout;
    Resource<Texture> skybox_texture;

    Fence immediate_fence;
    CommandBuffer immediate_command_buffer;
    CommandPool immediate_command_pool;

    AllocatedImage image_white;
    AllocatedImage image_black;
    AllocatedImage image_default_normal;
    AllocatedImage image_error;
    
    Sampler sampler_default_linear;
    Sampler sampler_default_nearest;

    DescriptorSetLayout single_image_descriptor_layout;
    
    ShaderObject skybox_shader;
    ShaderObject lighting_shader;
    ShaderObject skinning_shader;

    ktxVulkanDeviceInfo ktx_device_info;
    
    bool create_vulkan_instance(uint32_t p_extension_count, const char* const* p_extensions);
    bool create_physical_device();
    bool create_device();
    bool create_allocator();
    bool create_swapchain();
    bool create_draw_image();
    bool create_image_views();
    bool create_shader_objects();
    bool create_skybox_shader();
    bool create_lighting_shader();
    bool create_skinning_shader();
    bool create_commands();
    bool create_sync_objects();
    bool create_descriptors();
    void init_default_data();
    
    AllocatedBuffer create_buffer(size_t allocation_size, BufferUsageFlags usage, VmaMemoryUsage memory_usage);
    void destroy_buffer(const AllocatedBuffer& buffer);

    void cleanup_swapchain();
    bool init_imgui();
    
    void transition_image(CommandBuffer p_cmd, Image p_image, ImageLayout p_current_layout, ImageLayout p_target_layout, ImageAspectFlags p_aspect_flags = ImageAspectFlagBits::eNone);
    void copy_image_to_image(CommandBuffer p_cmd, Image p_source, Image p_destination, Extent2D source_size, Extent2D destination_size);
    void immediate_submit(std::function<void(CommandBuffer p_cmd)>&& function);

    void draw_skybox(CommandBuffer p_cmd, uint p_swapchain_image_index);
    void draw_geometry(CommandBuffer p_cmd);
    void draw_lighting(CommandBuffer p_cmd);
    void draw_imgui(CommandBuffer p_cmd, ImageView p_target_image_view);

public:
    SDL_Window* window{ nullptr };
    Instance instance;
    PhysicalDevice physical_device;
    Device device;
    SurfaceKHR surface;
    DrawContext draw_context;
    std::unordered_map<std::string, Ref<Node>> nodes;

    AllocatedImage gbuffer_albedo {};
    AllocatedImage gbuffer_normal {};
    AllocatedImage depth_image_ms {};
    AllocatedImage depth_image {};
    Extent2D draw_extent;

    AllocatedImage storage_image {};

    uint32_t flags {0};
    float white_point {1.0f};
    bool resize_requested {false};
    SceneLightsData scene_data;
    DescriptorSetLayout scene_data_descriptor_layout;

    MaterialInstance default_material {};
    MaterialMetallicRoughness metal_roughness_material {};
    
    FrameData& get_current_frame();
    bool initialize(uint32_t p_extension_count, const char* const* p_extensions, SDL_Window* p_window, uint32_t width, uint32_t height);
    
    bool create_shader_module(const uint32_t bytes[], const int length, ShaderModule &r_shader_module);
    GPUMeshBuffers upload_mesh(std::span<uint32_t> p_indices, std::span<Vertex> p_vertices);
    GPUMeshBuffers upload_mesh(std::span<uint32_t> p_indices, std::span<Vertex> p_vertices, std::span<SkinningData> p_skinning_data, std::span<Mat4> p_joint_matrices);
    AllocatedImage create_image(Extent3D p_size, Format p_format, ImageUsageFlags p_usage, bool mipmapped = False, SampleCountFlagBits p_sample_count = SampleCountFlagBits::e1);
    AllocatedImage create_image(void* p_data, Extent3D p_size, Format p_format, ImageUsageFlags p_usage, bool mipmapped = False, SampleCountFlagBits p_sample_count = SampleCountFlagBits::e1);
    void generate_mipmaps(CommandBuffer p_cmd, Image p_image, Extent2D p_size);
    void destroy_image(const AllocatedImage& p_image);

    void recreate_swapchain();
    void update_scene();
    void draw();
    void cleanup();
    
    Renderer() {};
    ~Renderer() {};
};
