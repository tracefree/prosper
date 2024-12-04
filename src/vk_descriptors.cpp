#include "vk_descriptors.h"

#include "util.h"
#include <vulkan/vk_enum_string_helper.h>

const uint32_t MAX_SETS_PER_POOL = 4092;


// --- DescriptorLayoutBuilder ---

void DescriptorLayoutBuilder::add_binding(uint32_t p_binding, DescriptorType p_type) {
    DescriptorSetLayoutBinding new_bind {
        .binding = p_binding,
        .descriptorType = p_type,
        .descriptorCount = 1,
    };
    bindings.push_back(new_bind);
}

void DescriptorLayoutBuilder::clear() {
    bindings.clear();
}

DescriptorSetLayout DescriptorLayoutBuilder::build(Device p_device, ShaderStageFlags p_shader_stages, void* p_pNext, DescriptorSetLayoutCreateFlags p_flags) {
    for (auto& binding : bindings) {
        binding.stageFlags |= p_shader_stages;
    }
    DescriptorSetLayoutCreateInfo create_info {
        .pNext = p_pNext,
        .flags = p_flags,
        .bindingCount = (uint32_t) bindings.size(),
        .pBindings    = bindings.data(),
    };
    DescriptorSetLayout set {};
    Result result;
    std::tie(result, set) = p_device.createDescriptorSetLayout(create_info);
    return set;
}


// --- DescriptorAllocator ---

void DescriptorAllocator::init_pool(Device p_device, uint32_t p_max_sets, std::span<PoolSizeRatio> p_pool_ratios) {
    std::vector<DescriptorPoolSize> pool_sizes;
    for (PoolSizeRatio ratio : p_pool_ratios) {
        pool_sizes.emplace_back(DescriptorPoolSize {
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * p_max_sets),
        });
    }

    DescriptorPoolCreateInfo pool_info {
        .maxSets       = p_max_sets,
        .poolSizeCount = (uint32_t) pool_sizes.size(),
        .pPoolSizes    = pool_sizes.data(),
    };

    Result result;
    std::tie(result, pool) = p_device.createDescriptorPool(pool_info);
}

void DescriptorAllocator::clear_descriptors(Device p_device) {
    p_device.resetDescriptorPool(pool);
}

void DescriptorAllocator::destroy_pool(Device p_device) {
    p_device.destroyDescriptorPool(pool);
}

DescriptorSet DescriptorAllocator::allocate(Device p_device, DescriptorSetLayout p_layout) {
    DescriptorSetAllocateInfo alloc_info {
        .descriptorPool     = pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &p_layout,
    };

    Result result;
    std::vector<DescriptorSet> descriptor_sets;
    std::tie(result, descriptor_sets) = p_device.allocateDescriptorSets(alloc_info);
    return descriptor_sets[0];
}


// --- DescriptorAllocatorGrowable ---

void DescriptorAllocatorGrowable::init_pool(Device p_device, uint32_t p_initial_sets, std::span<PoolSizeRatio> p_pool_ratios) {
    ratios.clear();
    for (auto ratio : p_pool_ratios) {
        ratios.push_back(ratio);
    }
    
    ready_pools.emplace_back(create_pool(p_device, p_initial_sets, p_pool_ratios));
    sets_per_pool = p_initial_sets * 1.5;
}

void DescriptorAllocatorGrowable::clear_pools(Device p_device) {
    for (auto pool : ready_pools) {
        p_device.resetDescriptorPool(pool);
    }

    for (auto pool : full_pools) {
        p_device.resetDescriptorPool(pool);
        ready_pools.push_back(pool);
    }
    full_pools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(Device p_device) {
    for (auto pool : ready_pools) {
        p_device.destroyDescriptorPool(pool);
    }
    ready_pools.clear();

    for (auto pool : full_pools) {
        p_device.destroyDescriptorPool(pool);
    }
    full_pools.clear();
}

DescriptorSet DescriptorAllocatorGrowable::allocate(Device p_device, DescriptorSetLayout p_layout, void* p_pNext) {
    DescriptorPool pool_to_use = get_pool(p_device);

    DescriptorSetAllocateInfo alloc_info {
        .pNext              = p_pNext,
        .descriptorPool     = pool_to_use,
        .descriptorSetCount = 1,
        .pSetLayouts        = &p_layout,
    };

    Result result;
    std::vector<DescriptorSet> descriptor_sets;
    std::tie(result, descriptor_sets) = p_device.allocateDescriptorSets(alloc_info);

    if (result == Result::eErrorOutOfPoolMemory || result == Result::eErrorFragmentedPool) {
        full_pools.push_back(pool_to_use);
        pool_to_use = get_pool(p_device);
        alloc_info.descriptorPool = pool_to_use;
        std::tie(result, descriptor_sets) = p_device.allocateDescriptorSets(alloc_info);
    }

    if (result != Result::eSuccess) {
        print("Could not allocate descriptor pool! Vulkan error: %s", string_VkResult((VkResult) result));
        return VK_NULL_HANDLE;
    }

    ready_pools.push_back(pool_to_use);
    return descriptor_sets[0];
}

DescriptorPool DescriptorAllocatorGrowable::get_pool(Device p_device) {
    DescriptorPool new_pool;
    if (ready_pools.size() != 0) {
        new_pool = ready_pools.back();
        ready_pools.pop_back();
    } else {
        new_pool = create_pool(p_device, sets_per_pool, ratios);
        sets_per_pool *= 1.5;
        if (sets_per_pool > MAX_SETS_PER_POOL) {
            sets_per_pool = MAX_SETS_PER_POOL;
        }
    }

    return new_pool;
}

DescriptorPool DescriptorAllocatorGrowable::create_pool(Device p_device, uint32_t set_count, std::span<PoolSizeRatio> p_pool_ratios) {
    std::vector<DescriptorPoolSize> pool_sizes;
    for (PoolSizeRatio ratio : p_pool_ratios) {
        pool_sizes.emplace_back(DescriptorPoolSize {
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * set_count),    
        });
    }

    DescriptorPoolCreateInfo pool_info {
        .maxSets       = set_count,
        .poolSizeCount = (uint32_t) pool_sizes.size(),
        .pPoolSizes    = pool_sizes.data(),
    };

    Result result;
    DescriptorPool new_pool;
    std::tie(result, new_pool) = p_device.createDescriptorPool(pool_info);
    
    if (result != Result::eSuccess) {
        print("Could not create descriptor pool! Vulkan error: %s", string_VkResult((VkResult) result));
        return VK_NULL_HANDLE;
    }

    return new_pool;
}


// --- DescriptorWriter ---

void DescriptorWriter::write_image(uint32_t p_binding, ImageView p_image_view, Sampler p_sampler, ImageLayout p_layout, DescriptorType p_type) {
    DescriptorImageInfo& image_info = image_infos.emplace_back(DescriptorImageInfo {
        .sampler = p_sampler,
        .imageView = p_image_view,
        .imageLayout = p_layout,
    });

    writes.emplace_back(WriteDescriptorSet {
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = p_binding,
        .descriptorCount = 1,
        .descriptorType = p_type,
        .pImageInfo = &image_info,
    });
}

void DescriptorWriter::write_buffer(uint32_t p_binding, Buffer p_buffer, size_t p_size, size_t p_offset, DescriptorType p_type) {
    DescriptorBufferInfo& buffer_info = buffer_infos.emplace_back(DescriptorBufferInfo {
        .buffer = p_buffer,
        .offset = p_offset,
        .range  = p_size,
    });

    writes.emplace_back(WriteDescriptorSet {
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = p_binding,
        .descriptorCount = 1,
        .descriptorType = p_type,
        .pBufferInfo = &buffer_info,
    });
}

void DescriptorWriter::clear() {
    image_infos.clear();
    writes.clear();
    buffer_infos.clear();
}

void DescriptorWriter::update_set(Device p_device, DescriptorSet p_set) {
    for (WriteDescriptorSet& write : writes) {
        write.dstSet = p_set;
    }
    p_device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
}