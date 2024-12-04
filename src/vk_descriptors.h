#pragma once

#include "vk_mem_alloc.h"
#include "vk_types.h"
#include <deque>

using namespace vk;

struct DescriptorLayoutBuilder {
private:
    std::vector<DescriptorSetLayoutBinding> bindings;

public:
    void add_binding(uint32_t p_binding, DescriptorType p_type);
    void clear();
    
    DescriptorSetLayout build(Device p_device, ShaderStageFlags p_shader_stages, void* p_pNext = nullptr, DescriptorSetLayoutCreateFlags p_flags = {});
};

struct DescriptorAllocator {
    struct PoolSizeRatio {
        DescriptorType type;
        float ratio;
    };

    DescriptorPool pool;

    void init_pool(Device p_device, uint32_t p_max_sets, std::span<PoolSizeRatio> p_pool_ratios);
    void clear_descriptors(Device p_device);
    void destroy_pool(Device p_device);
    
    DescriptorSet allocate(Device p_device, DescriptorSetLayout p_layout);
};

struct DescriptorAllocatorGrowable {
    struct PoolSizeRatio {
        DescriptorType type;
        float ratio;
    };

    void init_pool(Device p_device, uint32_t p_initial_sets, std::span<PoolSizeRatio> p_pool_ratios);
    void clear_pools(Device p_device);
    void destroy_pools(Device p_device);
    
    DescriptorSet allocate(Device p_device, DescriptorSetLayout p_layout, void* p_pNext = nullptr);

private:
    std::vector<PoolSizeRatio> ratios;
    std::vector<DescriptorPool> full_pools;
    std::vector<DescriptorPool> ready_pools;
    uint32_t sets_per_pool;
    
    DescriptorPool get_pool(Device p_device);
    DescriptorPool create_pool(Device p_device, uint32_t set_count, std::span<PoolSizeRatio> p_pool_ratios);

};

struct DescriptorWriter {
    std::deque<DescriptorImageInfo>  image_infos;
    std::deque<DescriptorBufferInfo> buffer_infos;
    std::vector<WriteDescriptorSet>  writes;

    void write_image(uint32_t p_binding, ImageView p_image_view, Sampler p_sampler, ImageLayout p_layout, DescriptorType p_type);
    void write_buffer(uint32_t p_binding, Buffer p_buffer, size_t p_size, size_t p_offset, DescriptorType p_type);

    void clear();
    void update_set(Device p_device, DescriptorSet p_set);
};