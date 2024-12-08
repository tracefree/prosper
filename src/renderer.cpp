#include "renderer.h"
#include "vk_pipelines.h"
#include "util.h"
#include "math.h"
#include "camera.h"

#include "shaders/gradient.h"
#include "shaders/mesh.h"
#include "shaders/skybox.h"

#include <cmath>
#include <vulkan/vk_enum_string_helper.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

extern Camera gCamera;
extern PerformanceStats gStats;
extern bool gValidationLayersEnabled;

bool Renderer::create_vulkan_instance(uint32_t p_extension_count, const char* const* p_extensions) {
    ApplicationInfo application_info {
        .apiVersion = ApiVersion13,
    };
    
    std::vector<const char*> enabled_layers;
    if (gValidationLayersEnabled) {
        enabled_layers.emplace_back("VK_LAYER_KHRONOS_validation");
    }

    InstanceCreateInfo create_info {
        .pApplicationInfo = &application_info,
        .enabledLayerCount = uint32_t(enabled_layers.size()),
        .ppEnabledLayerNames = enabled_layers.data(),
        .enabledExtensionCount = p_extension_count,
        .ppEnabledExtensionNames = p_extensions,
    };

    Result result;
    std::tie(result, instance) = createInstance(create_info, nullptr);
    deletion_queue.push_function([this]() {
        instance.destroy();
    });
    return result == Result::eSuccess;
}

bool Renderer::create_physical_device() {
    Result result;
    std::vector<PhysicalDevice> physical_devices;
    std::tie(result, physical_devices) = instance.enumeratePhysicalDevices();
    if (result != Result::eSuccess || physical_devices.size() == 0) {
        print("Could not find a physical rendering device!");
        return false;
    }
    physical_device = physical_devices[0];
    return true;
}


bool Renderer::create_device() {
    std::vector<QueueFamilyProperties> queue_family = physical_device.getQueueFamilyProperties();
    graphics_queue_index = 0;
    for (const QueueFamilyProperties &queue_family : queue_family) {
        if (queue_family.queueFlags & QueueFlagBits::eGraphics) {
            Bool32 presentation_support = false;
            Result result = physical_device.getSurfaceSupportKHR(graphics_queue_index, surface, &presentation_support);
            if (result != Result::eSuccess) {
                return false;
            }
            // TODO: Bool32 graphics_support
            if (presentation_support) {
                break;
            }
        }
        graphics_queue_index++;
    }
    
    const float priority = 1.0f;
    std::vector<const char*> enabled_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_SHADER_OBJECT_EXTENSION_NAME
    };
    DeviceQueueCreateInfo queue_create_info {
        .queueFamilyIndex = graphics_queue_index,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    PhysicalDeviceFeatures features {};
    PhysicalDeviceShaderObjectFeaturesEXT shader_object_extension { .shaderObject = True };
    PhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features {
        .pNext = &shader_object_extension,
        .bufferDeviceAddress = True,
    };
    PhysicalDeviceVulkan13Features features13 {
        .pNext = &buffer_device_address_features,
        .synchronization2 = True,
        .dynamicRendering = True,
    };
    PhysicalDeviceFeatures2 features2 {
        .pNext = &features13,
        .features = features,
    };
    
    DeviceCreateInfo create_info {
        .pNext = &features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = uint32_t(enabled_extensions.size()),
        .ppEnabledExtensionNames = enabled_extensions.data(),
    };
    Result result;
    std::tie(result, device) = physical_device.createDevice(create_info);
    deletion_queue.push_function([this]() {
        device.destroy();
    });

    graphics_queue = device.getQueue(graphics_queue_index, 0);
    return result == Result::eSuccess;
}

bool Renderer::create_allocator() {
    VmaAllocatorCreateInfo allocator_info {
        .flags          = VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physical_device,
        .device         = device,
        .instance       = instance,
    };
    VkResult result = vmaCreateAllocator(&allocator_info, &allocator);
    deletion_queue.push_function([&]() {
        vmaDestroyAllocator(allocator);
    });
    return result == VK_SUCCESS;
}

bool Renderer::create_swapchain() {
    Result result;

    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    viewport_size.width = w;
    viewport_size.height = h;

    SwapchainCreateInfoKHR create_info {
        .surface = surface,
        .minImageCount = 3,
        .imageFormat = swapchain_image_format,
        .imageExtent = viewport_size,
        .imageArrayLayers = 1,
        .imageUsage = ImageUsageFlagBits::eColorAttachment | ImageUsageFlagBits::eTransferDst,
        .presentMode = PresentModeKHR::eFifo,
        .clipped = True
    };
    std::tie(result, swapchain) = device.createSwapchainKHR(create_info);

    return result == Result::eSuccess;
}

bool Renderer::create_draw_image() {
    // TODO: Don't hardcode monitor resolution
    Extent3D draw_image_extent {
        .width  = 2560,
        .height = 1440,
        .depth  = 1,
    };

    draw_image.image_format = Format::eR16G16B16A16Sfloat;
    draw_image.image_extent = draw_image_extent;

    ImageUsageFlags draw_image_usages = ImageUsageFlagBits::eTransferSrc | ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eStorage | ImageUsageFlagBits::eColorAttachment;
    ImageCreateInfo image_info {
        .imageType   = ImageType::e2D,
        .format      = draw_image.image_format,
        .extent      = draw_image_extent,
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = SampleCountFlagBits::e1,
        .tiling      = ImageTiling::eOptimal,
        .usage       = draw_image_usages,
    };

    VmaAllocationCreateInfo image_alloc_info {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    
    VkResult creation_result = vmaCreateImage(allocator, (VkImageCreateInfo*)&image_info, &image_alloc_info, (VkImage_T**) &draw_image.image, &draw_image.allocation, nullptr);
    if (creation_result != VK_SUCCESS) {
        return false;
    }

    ImageViewCreateInfo view_info {
        .image      = draw_image.image,
        .viewType   = ImageViewType::e2D,
        .format     = draw_image.image_format,
        .subresourceRange = ImageSubresourceRange {
            .aspectMask     = ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        }
    };
    Result result;
    std::tie(result, draw_image.image_view) = device.createImageView(view_info);
    
    depth_image.image_format = Format::eD32Sfloat;
    depth_image.image_extent = draw_image.image_extent;
    ImageUsageFlags depth_image_usages {ImageUsageFlagBits::eDepthStencilAttachment};
    image_info.format = depth_image.image_format;
    image_info.usage  = depth_image_usages;
    creation_result = vmaCreateImage(allocator, (VkImageCreateInfo*)&image_info, &image_alloc_info, (VkImage_T**) &depth_image.image, &depth_image.allocation, nullptr);
    
    view_info.image = depth_image.image;
    view_info.format = depth_image.image_format;
    view_info.subresourceRange.aspectMask = ImageAspectFlagBits::eDepth;
    std::tie(result, depth_image.image_view) = device.createImageView(view_info);

    deletion_queue.push_function([this]() {
        vmaDestroyImage(allocator, draw_image.image, draw_image.allocation);
        device.destroyImageView(draw_image.image_view);

        vmaDestroyImage(allocator, depth_image.image, depth_image.allocation);
        device.destroyImageView(depth_image.image_view);
    });
    return (result == Result::eSuccess);
}


bool Renderer::create_image_views() {
    uint32_t swapchain_image_count = 0;
    Result result = device.getSwapchainImagesKHR(swapchain, &swapchain_image_count, nullptr);
    swapchain_images.resize(swapchain_image_count);
    result = device.getSwapchainImagesKHR(swapchain, &swapchain_image_count, swapchain_images.data());
    swapchain_image_views.resize(swapchain_image_count);

    for (int i = 0; i < swapchain_image_count; i++) {
        ImageViewCreateInfo create_info {
            .image = swapchain_images[i],
            .viewType = ImageViewType::e2D,
            .format = swapchain_image_format,
            .components = ComponentMapping(),
            .subresourceRange = ImageSubresourceRange {
                .aspectMask     = ImageAspectFlagBits::eColor,
                .baseMipLevel   = 0,
                .levelCount     = RemainingMipLevels,
                .baseArrayLayer = 0,
                .layerCount     = RemainingArrayLayers,
            }
        };
        Result result = device.createImageView(&create_info, nullptr, &swapchain_image_views[i]);
        if (result != Result::eSuccess) {
            return false;
        }
    }
    return true;
}

FrameData& Renderer::get_current_frame() {
    return frames[current_frame % MAX_FRAMES_IN_FLIGHT];
}


bool Renderer::create_shader_module(const uint32_t bytes[], const int length, ShaderModule &r_shader_module) {
    ShaderModuleCreateInfo shader_info {
        .codeSize = (long unsigned int) length,
        .pCode    = bytes,
    };

    Result result;
    std::tie(result, r_shader_module) = device.createShaderModule(shader_info);
    return result == Result::eSuccess;
}


bool Renderer::create_pipelines() {
    create_background_shader();
    metal_roughness_material.build_pipelines(this);
    
    return true;
}


bool Renderer::create_background_shader() {
    PushConstantRange push_constants {
        .stageFlags = ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(Mat4),
    };

    PipelineLayoutCreateInfo layout_info {
        .setLayoutCount = 1,
        .pSetLayouts    = &draw_image_descriptor_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constants,
    };

    Result result;
    std::tie(result, skybox_layout) = device.createPipelineLayout(layout_info);
    if (result != Result::eSuccess) {
        print("Could not create pipeline layout!");
        return false;
    }

    auto getInstanceProcAddress = (PFN_vkGetInstanceProcAddr) SDL_Vulkan_GetVkGetInstanceProcAddr();
    dispatch_loader = DispatchLoaderDynamic(instance, getInstanceProcAddress, device);

    ShaderCreateInfoEXT vertex_shader_info {
        .flags = ShaderCreateFlagBitsEXT::eLinkStage,
        .stage = ShaderStageFlagBits::eVertex,
        .nextStage = ShaderStageFlagBits::eFragment,
        .codeType = ShaderCodeTypeEXT::eSpirv,
        .codeSize = skybox_spv_sizeInBytes,
        .pCode = skybox_spv,
        .pName = "vertex",
        .setLayoutCount = 1,
        .pSetLayouts = &draw_image_descriptor_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constants,
    };

    ShaderCreateInfoEXT fragment_shader_info = vertex_shader_info;
    fragment_shader_info.stage = ShaderStageFlagBits::eFragment;
    fragment_shader_info.nextStage = {};
    fragment_shader_info.pName = "fragment";


    std::vector<ShaderCreateInfoEXT> shader_infos = { vertex_shader_info, fragment_shader_info };
    std::vector<ShaderEXT> skybox_shaders;
    skybox_shaders = device.createShadersEXT(shader_infos, nullptr, dispatch_loader).value;
    skybox_shader.vertex = skybox_shaders[0];
    skybox_shader.fragment = skybox_shaders[1];
    
    deletion_queue.push_function([&]() {
        device.destroyShaderEXT(skybox_shader.vertex, nullptr, dispatch_loader);
        device.destroyShaderEXT(skybox_shader.fragment, nullptr, dispatch_loader);
        device.destroyPipelineLayout(skybox_layout);
    });

    return true;
}


bool Renderer::create_commands() {
    CommandPoolCreateInfo create_info {
        .flags = CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphics_queue_index,
    };

    Result result;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::tie(result, frames[i].command_pool) = device.createCommandPool(create_info);
        if (result != Result::eSuccess) {
            return false;
        }

        CommandBufferAllocateInfo command_alloc_info {
            .commandPool = frames[i].command_pool,
            .commandBufferCount = 1,
        };

        std::vector<CommandBuffer> buffers;
        std::tie(result, buffers) = device.allocateCommandBuffers(command_alloc_info);
        if (result != Result::eSuccess) { return false; }
        frames[i].command_buffer = buffers[0];
    }

    // Immediate commands
    std::tie(result, immediate_command_pool) = device.createCommandPool(create_info);
    if (result != Result::eSuccess) { return false; }
    CommandBufferAllocateInfo command_alloc_info {
        .commandPool        = immediate_command_pool,
        .commandBufferCount = 1,
    };

    std::vector<CommandBuffer> buffers;
    std::tie(result, buffers) = device.allocateCommandBuffers(command_alloc_info);
    if (result != Result::eSuccess) { return false; }
    immediate_command_buffer = buffers[0];

    deletion_queue.push_function([this]() {
        device.destroyCommandPool(immediate_command_pool);
    });

    return true;
}


bool Renderer::create_sync_objects() {
    FenceCreateInfo fence_create_info {
        .flags = FenceCreateFlagBits::eSignaled,
    };

    SemaphoreCreateInfo semaphore_create_info {};
    
    Result result;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::tie(result, frames[i].render_fence) = device.createFence(fence_create_info);
        if (result != Result::eSuccess) { return false; }

        std::tie(result, frames[i].swapchain_semaphore) = device.createSemaphore(semaphore_create_info);
        if (result != Result::eSuccess) { return false; }
        
        std::tie(result, frames[i].render_semaphore) = device.createSemaphore(semaphore_create_info);
        if (result != Result::eSuccess) { return false; }
    }

    std::tie(result, immediate_fence) = device.createFence(fence_create_info);
    deletion_queue.push_function([this]() {
        device.destroyFence(immediate_fence);
    });
    return result == Result::eSuccess;
}


bool Renderer::create_descriptors() {
    std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
        { DescriptorType::eStorageImage, 1}
    };

    global_descriptor_allocator.init_pool(device, 10, sizes);

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, DescriptorType::eStorageImage);
        draw_image_descriptor_layout = builder.build(device, ShaderStageFlagBits::eCompute);
    }

    draw_image_descriptors = global_descriptor_allocator.allocate(device, draw_image_descriptor_layout);
    
    DescriptorWriter writer {};
    writer.write_image(0, draw_image.image_view, VK_NULL_HANDLE, ImageLayout::eGeneral, DescriptorType::eStorageImage);
    writer.update_set(device, draw_image_descriptors);

    DescriptorLayoutBuilder builder;
    builder.add_binding(0, DescriptorType::eUniformBuffer);
    scene_data_descriptor_layout = builder.build(device, ShaderStageFlagBits::eVertex | ShaderStageFlagBits::eFragment);

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, DescriptorType::eCombinedImageSampler);
        single_image_descriptor_layout = builder.build(device, ShaderStageFlagBits::eFragment);
    }
    
    deletion_queue.push_function([&]() {
        global_descriptor_allocator.destroy_pools(device);
        device.destroyDescriptorSetLayout(draw_image_descriptor_layout);
        device.destroyDescriptorSetLayout(scene_data_descriptor_layout);
        device.destroyDescriptorSetLayout(single_image_descriptor_layout);
    });

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            {DescriptorType::eStorageImage, 3},
            {DescriptorType::eStorageBuffer, 3},
            {DescriptorType::eUniformBuffer, 3},
            {DescriptorType::eCombinedImageSampler, 4},
        };

        frames[i].descriptors = DescriptorAllocatorGrowable {};
        frames[i].descriptors.init_pool(device, 1000, frame_sizes);
        deletion_queue.push_function([&, i]() {
            frames[i].descriptors.destroy_pools(device);
        });
    }

    return true;
}


void Renderer::init_default_data() {
    // Default textures
    uint32_t white = glm::packUnorm4x8(glm::vec4(1.0f));
    image_white = create_image((void*) &white, Extent3D {1, 1, 1}, Format::eR8G8B8A8Unorm, ImageUsageFlagBits::eSampled);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    image_black = create_image((void*) &white, Extent3D {1, 1, 1}, Format::eR8G8B8A8Unorm, ImageUsageFlagBits::eSampled);
    
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 * 16 > pixels;
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
    image_error = create_image(pixels.data(), Extent3D {16, 16, 1}, Format::eR8G8B8A8Unorm, ImageUsageFlagBits::eSampled);

    SamplerCreateInfo sample_info {
        .magFilter = Filter::eNearest,
        .minFilter = Filter::eNearest,
        .mipmapMode = SamplerMipmapMode::eNearest,
    };
    Result result;
    std::tie(result, sampler_default_nearest) = device.createSampler(sample_info);

    sample_info.magFilter = sample_info.minFilter = Filter::eLinear;
    std::tie(result, sampler_default_linear) = device.createSampler(sample_info);

    MaterialMetallicRoughness::MaterialResources material_resources {
        .albedo_image            = image_white,
        .albedo_sampler          = sampler_default_linear,
        .metal_roughness_image   = image_black,
        .metal_roughness_sampler = sampler_default_linear,
    };

    AllocatedBuffer material_constants = create_buffer(sizeof(MaterialMetallicRoughness::MaterialConstants), BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

    MaterialMetallicRoughness::MaterialConstants* scene_uniform_data = (MaterialMetallicRoughness::MaterialConstants*) material_constants.info.pMappedData;
    scene_uniform_data->albedo_factors = Vec4(1.0);
    scene_uniform_data->metal_roughness_factors = Vec4(0.0, 0.5, 0.0, 0.0);

    material_resources.data_buffer = material_constants.buffer;
    material_resources.data_buffer_offset = 0;
    default_material = metal_roughness_material.write_material(device, MaterialPass::MainColor, material_resources, global_descriptor_allocator);

    deletion_queue.push_function([=, this]() {
        destroy_buffer(material_constants);

        for (auto mesh_ptr : test_meshes) {
            destroy_buffer(mesh_ptr.get()->mesh_buffers.index_buffer);
            destroy_buffer(mesh_ptr.get()->mesh_buffers.vertex_buffer);
        }

        destroy_image(image_white);
        destroy_image(image_black);
        destroy_image(image_error);

        device.destroySampler(sampler_default_linear);
        device.destroySampler(sampler_default_nearest);
    });
}


AllocatedBuffer Renderer::create_buffer(size_t p_allocation_size, BufferUsageFlags usage, VmaMemoryUsage memory_usage) {
    BufferCreateInfo buffer_info {
        .size = p_allocation_size,
        .usage = usage,
    };
    VmaAllocationCreateInfo vma_alloc_info {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memory_usage,
    };
    
    AllocatedBuffer new_buffer;
    VkResult result = vmaCreateBuffer(
        allocator,
        (VkBufferCreateInfo*) &buffer_info,
        &vma_alloc_info,
        (VkBuffer*) &new_buffer.buffer,
        &new_buffer.allocation,
        &new_buffer.info
    );
    return new_buffer;
}

void Renderer::destroy_buffer(const AllocatedBuffer& p_buffer) {
    vmaDestroyBuffer(allocator, (VkBuffer) p_buffer.buffer, p_buffer.allocation);
}


AllocatedImage Renderer::create_image(Extent3D p_size, Format p_format, ImageUsageFlags p_usage, bool mipmapped) {
    AllocatedImage new_image {
        .image_format = p_format,
        .image_extent = p_size,
    };

    ImageCreateInfo image_info {
        .imageType   = ImageType::e2D,
        .format = p_format,
        .extent = p_size,
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = SampleCountFlagBits::e1,
        .tiling      = ImageTiling::eOptimal,
        .usage = p_usage,
    };

    if (mipmapped) {
        image_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(p_size.width, p_size.height)))) + 1;
    }

    VmaAllocationCreateInfo alloc_info {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = (VkMemoryPropertyFlags) MemoryPropertyFlagBits::eDeviceLocal,
    };

    VkResult image_result = vmaCreateImage(allocator, (VkImageCreateInfo*) &image_info, &alloc_info, (VkImage_T**) &new_image.image, &new_image.allocation, nullptr);
    if (image_result != VK_SUCCESS) {
        print("Could not create image! VMA error: %s", string_VkResult((VkResult) image_result));
        return {};
    }

    ImageAspectFlags aspect_flags = (p_format == Format::eD32Sfloat) ? ImageAspectFlagBits::eDepth : ImageAspectFlagBits::eColor;
    ImageViewCreateInfo view_info {
        .image      = new_image.image,
        .viewType   = ImageViewType::e2D,
        .format     = p_format,
        .subresourceRange = ImageSubresourceRange {
            .aspectMask     = aspect_flags,
            .baseMipLevel   = 0,
            .levelCount     = image_info.mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        }
    };

    Result image_view_result;
    std::tie(image_view_result, new_image.image_view) = device.createImageView(view_info);
    if (image_view_result != Result::eSuccess) {
        print("Could not create image view!");
        return {};
    }
    return new_image;
}

AllocatedImage Renderer::create_image(void* p_data, Extent3D p_size, Format p_format, ImageUsageFlags p_usage, bool p_mipmapped) {
    size_t data_size = p_size.depth * p_size.width * p_size.height * 4;
    AllocatedBuffer upload_buffer = create_buffer(data_size, BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU);
    memcpy(upload_buffer.info.pMappedData, p_data, data_size);
    AllocatedImage new_image = create_image(p_size, p_format, p_usage | ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eTransferSrc, p_mipmapped);
    
    immediate_submit([&, this](CommandBuffer cmd) {
        transition_image(cmd, new_image.image, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal);
        BufferImageCopy copy_region {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageExtent = p_size,
        };

        cmd.copyBufferToImage(upload_buffer.buffer, new_image.image, ImageLayout::eTransferDstOptimal, 1, &copy_region);

        if (p_mipmapped) {
            generate_mipmaps(cmd, new_image.image, Extent2D {new_image.image_extent.width, new_image.image_extent.height});
        } else {
            transition_image(cmd, new_image.image, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal);
        }
    });

    destroy_buffer(upload_buffer);
    return new_image;
}

void Renderer::generate_mipmaps(CommandBuffer p_cmd, Image p_image, Extent2D p_size) {
    uint32_t mip_levels = uint32_t(std::floor(std::log2(std::max(p_size.width, p_size.height)))) + 1;
    for (uint32_t mip_level = 0; mip_level < mip_levels; mip_level++) {
        Extent2D half_size {
            .width  = uint32_t(p_size.width / 2),
            .height = uint32_t(p_size.height / 2),
        };

        ImageMemoryBarrier2 image_barrier {
            .srcStageMask = PipelineStageFlagBits2::eAllCommands,
            .srcAccessMask = AccessFlagBits2::eMemoryWrite,
            .dstStageMask = PipelineStageFlagBits2::eAllCommands,
            .dstAccessMask = AccessFlagBits2::eMemoryWrite | AccessFlagBits2::eMemoryRead,
            .oldLayout = ImageLayout::eTransferDstOptimal,
            .newLayout = ImageLayout::eTransferSrcOptimal,
            .image = p_image,
            .subresourceRange = {
                .aspectMask = ImageAspectFlagBits::eColor,
                .baseMipLevel = mip_level,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = RemainingArrayLayers,
            },
        };

        DependencyInfo dependency_info {
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &image_barrier
        };

        p_cmd.pipelineBarrier2(dependency_info);

        if (mip_level < mip_levels - 1) {
            ImageBlit2 blit_region {
                .srcSubresource = {
                    .aspectMask = ImageAspectFlagBits::eColor,
                    .mipLevel = mip_level,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .dstSubresource = {
                    .aspectMask = ImageAspectFlagBits::eColor,
                    .mipLevel = mip_level + 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            blit_region.srcOffsets[1] = {int(p_size.width), int(p_size.height), 1};
            blit_region.dstOffsets[1] = {int(half_size.width), int(half_size.height), 1};

            BlitImageInfo2 blit_info {
                .srcImage = p_image,
                .srcImageLayout = ImageLayout::eTransferSrcOptimal,
                .dstImage = p_image,
                .dstImageLayout = ImageLayout::eTransferDstOptimal,
                .regionCount = 1,
                .pRegions = &blit_region,
                .filter = Filter::eLinear,
            };

            p_cmd.blitImage2(blit_info);
            p_size = half_size;
        }
    }

    transition_image(p_cmd, p_image, ImageLayout::eTransferSrcOptimal, ImageLayout::eShaderReadOnlyOptimal);
}

void Renderer::destroy_image(const AllocatedImage& p_image) {
    device.destroyImageView(p_image.image_view, nullptr);
    vmaDestroyImage(allocator, p_image.image, p_image.allocation);
}


GPUMeshBuffers Renderer::upload_mesh(std::span<uint32_t> p_indices, std::span<Vertex> p_vertices) {
    const size_t index_buffer_size  = p_indices.size()  * sizeof(uint32_t);
    const size_t vertex_buffer_size = p_vertices.size() * sizeof(Vertex);

    GPUMeshBuffers new_surface;

    new_surface.vertex_buffer = create_buffer(
        vertex_buffer_size,
        BufferUsageFlagBits::eStorageBuffer | BufferUsageFlagBits::eTransferDst | BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    BufferDeviceAddressInfo device_address_info {
        .buffer = new_surface.vertex_buffer.buffer,
    };
    new_surface.vertex_buffer_address = device.getBufferAddress(device_address_info);

    new_surface.index_buffer = create_buffer(
        index_buffer_size,
        BufferUsageFlagBits::eIndexBuffer | BufferUsageFlagBits::eTransferDst,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    AllocatedBuffer staging = create_buffer(
        vertex_buffer_size + index_buffer_size,
        BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY
    );

    void* data = staging.info.pMappedData;

    memcpy(data, p_vertices.data(), vertex_buffer_size);
    memcpy((char*)data + vertex_buffer_size, p_indices.data(), index_buffer_size);

    immediate_submit([&](CommandBuffer cmd) {
        BufferCopy vertex_copy {
            .size      = vertex_buffer_size,
        };
        cmd.copyBuffer(staging.buffer, new_surface.vertex_buffer.buffer, 1, &vertex_copy);

        BufferCopy index_copy {
            .srcOffset = vertex_buffer_size,
            .size      = index_buffer_size,
        };
        cmd.copyBuffer(staging.buffer, new_surface.index_buffer.buffer, 1, &index_copy);
    });

    destroy_buffer(staging);

    return new_surface;
}


void Renderer::cleanup_swapchain() {
    device.waitIdle();
    device.destroySwapchainKHR(swapchain);
    for (int i = 0; i < swapchain_image_views.size(); i++) {
        device.destroyImageView(swapchain_image_views[i], nullptr);
    }
}


void Renderer::recreate_swapchain() {
    cleanup_swapchain();
    create_swapchain();
    create_image_views();
    resize_requested = false;
}


void Renderer::immediate_submit(std::function<void(CommandBuffer p_cmd)>&& function) {
    device.resetFences(1, &immediate_fence);
    immediate_command_buffer.reset();

    CommandBuffer cmd = immediate_command_buffer;

    CommandBufferBeginInfo begin_info {
        .flags = CommandBufferUsageFlagBits::eOneTimeSubmit,
    };

    cmd.begin(begin_info);

    function(cmd);

    cmd.end();

    CommandBufferSubmitInfo submit_info {
        .commandBuffer = cmd,
    };
    SubmitInfo2 submit {
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &submit_info,
    };
    graphics_queue.submit2(submit, immediate_fence);
    device.waitForFences(1, &immediate_fence, True, UINT64_MAX);
}


void Renderer::transition_image(CommandBuffer p_cmd, Image p_image, ImageLayout p_current_layout, ImageLayout p_target_layout) {
    ImageAspectFlags aspect_mask = (p_target_layout == ImageLayout::eDepthAttachmentOptimal) ? ImageAspectFlagBits::eDepth : ImageAspectFlagBits::eColor;
    
    ImageMemoryBarrier2 image_barrier {
        .srcStageMask   = PipelineStageFlagBits2::eAllCommands,
        .srcAccessMask  = AccessFlagBits2::eMemoryWrite,
        .dstStageMask   = PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask  = AccessFlagBits2::eMemoryWrite | AccessFlagBits2::eMemoryRead,

        .oldLayout = p_current_layout,
        .newLayout = p_target_layout,

        .image              = p_image,
        .subresourceRange   = ImageSubresourceRange {
            .aspectMask     = aspect_mask,
            .baseMipLevel   = 0,
            .levelCount     = RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount     = RemainingArrayLayers,
        },
    };

    DependencyInfo dependency_info {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier,
    };

    p_cmd.pipelineBarrier2(dependency_info);
}

void Renderer::copy_image_to_image(CommandBuffer p_cmd, Image p_source, Image p_destination, Extent2D source_size, Extent2D destination_size) {
    //std::vector<Offset3D> src_offsets = { Offset3D{source_size.width, source_size.height, 1} };
    ImageBlit2 blit_region {
        .srcSubresource = ImageSubresourceLayers {
            .aspectMask = ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstSubresource = ImageSubresourceLayers {
            .aspectMask = ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    blit_region.srcOffsets[1] = Offset3D {static_cast<int32_t>(source_size.width), static_cast<int32_t>(source_size.height), 1};
    blit_region.dstOffsets[1] = Offset3D {static_cast<int32_t>(destination_size.width), static_cast<int32_t>(destination_size.height), 1};

    BlitImageInfo2 blit_info {
        .srcImage       = p_source,
        .srcImageLayout = ImageLayout::eTransferSrcOptimal,
        .dstImage       = p_destination,
        .dstImageLayout = ImageLayout::eTransferDstOptimal,
        .regionCount    = 1,
        .pRegions       = &blit_region,
        .filter         = Filter::eLinear,
    };

    p_cmd.blitImage2(blit_info);
}


bool Renderer::init_imgui() {
    DescriptorPoolSize pool_sizes[] = {
        {DescriptorType::eSampler, 1000},
        {DescriptorType::eCombinedImageSampler, 1000},
        {DescriptorType::eSampledImage, 1000},
        {DescriptorType::eStorageImage, 1000},
        {DescriptorType::eUniformTexelBuffer, 1000},
        {DescriptorType::eStorageTexelBuffer, 1000},
        {DescriptorType::eUniformBuffer, 1000},
        {DescriptorType::eStorageBuffer, 1000},
        {DescriptorType::eUniformBufferDynamic, 1000},
        {DescriptorType::eStorageBufferDynamic, 1000},
        {DescriptorType::eInputAttachment, 1000},
    };
    DescriptorPoolCreateInfo pool_info {
        .flags         = DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 1000,
        .poolSizeCount = (uint32_t) std::size(pool_sizes),
        .pPoolSizes    = pool_sizes,
    };
    DescriptorPool imgui_pool;
    Result result;
    std::tie(result, imgui_pool) = device.createDescriptorPool(pool_info);

    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(window);

    ImGui_ImplVulkan_InitInfo imgui_info {
        .Instance            = instance,
        .PhysicalDevice      = physical_device,
        .Device              = device,
        .Queue               = graphics_queue,
        .DescriptorPool      = imgui_pool,
        .MinImageCount       = 3,
        .ImageCount          = 3,
        .MSAASamples = (VkSampleCountFlagBits) SampleCountFlagBits::e1,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = {
            .sType                   = (VkStructureType) StructureType::ePipelineRenderingCreateInfo,
            .colorAttachmentCount    = 1,
            .pColorAttachmentFormats = (VkFormat*)(&swapchain_image_format),
        },
    };

    ImGui_ImplVulkan_Init(&imgui_info);
    ImGui_ImplVulkan_CreateFontsTexture();

    deletion_queue.push_function([this, imgui_pool]() {
        ImGui_ImplVulkan_Shutdown();
        device.destroyDescriptorPool(imgui_pool);
    });

    return true;
}


bool Renderer::initialize(uint32_t p_extension_count, const char* const* p_extensions, SDL_Window* p_window, uint32_t p_width, uint32_t p_height) {
    window = p_window;
    viewport_size = Extent2D(p_width, p_height);
    draw_extent = viewport_size;
    swapchain_image_format = Format::eB8G8R8A8Srgb;

    
    if (!create_vulkan_instance(p_extension_count, p_extensions)) {
        print("Could not create Vulkan instance!");
    }
    
    VkSurfaceKHR vk_surface;
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &vk_surface)) {
        print("Could not create Vulkan surface! SDL error: %s", SDL_GetError());
        return false;
    }
    surface = vk_surface;
    deletion_queue.push_function([this]() {
        instance.destroySurfaceKHR(surface);
    });

    if (!create_physical_device()) {
        print("Could not create physical device!");
    }
    if (!create_device()) {
        print("Could not create logical device!");
        return false;
    }
    if (!create_allocator()) {
        print("Could not create memory allocator!");
        return false;
    }
    if(!create_swapchain()) {
        print("Could not create swapchain!");
        return false;
    }
    if(!create_draw_image()) {
        print("Could not create draw image!");
        return false;
    }
    if (!create_image_views()) {
        print("Could not create image views!");
        return false;
    }
    if (!create_commands()) {
        print("Could not create command pool!");
        return false;
    }
    if (!create_sync_objects()) {
        print("Could not create fence and semaphores!");
        return false;
    }
    if (!create_descriptors()) {
        print("Could not create descriptors!");
        return false;
    }
    if (!create_pipelines()) {
        print("Could not create pipelines!");
        return false;
    }
    if (!init_imgui()) {
        print("Could not initialize Dear ImGui!");
        return false;
    }
    init_default_data();

    return true;
}


void Renderer::cleanup() {
    cleanup_swapchain();

    loaded_scenes.clear();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frames[i].deletion_queue.flush();

        device.destroyCommandPool(frames[i].command_pool);
        device.destroyFence(frames[i].render_fence);
        device.destroySemaphore(frames[i].swapchain_semaphore);
        device.destroySemaphore(frames[i].render_semaphore);
    }
    metal_roughness_material.clear_resources(device);
    deletion_queue.flush();
    SDL_DestroyWindow(window);
}


void Renderer::draw_skybox(CommandBuffer p_cmd) {
    RenderingAttachmentInfo color_attachment {
        .imageView   = draw_image.image_view,
        .imageLayout = ImageLayout::eColorAttachmentOptimal,
        .loadOp      = AttachmentLoadOp::eLoad,
        .storeOp     = AttachmentStoreOp::eStore,
    };
    RenderingAttachmentInfo depth_attachment {
        .imageView   = depth_image.image_view,
        .imageLayout = ImageLayout::eDepthAttachmentOptimal,
        .loadOp      = AttachmentLoadOp::eLoad,
        .storeOp     = AttachmentStoreOp::eNone,
    };
    RenderingInfo render_info {
        .renderArea = Rect2D { {0, 0}, {draw_image.image_extent.width, draw_image.image_extent.height} },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
    };

    p_cmd.beginRendering(render_info);

    Mat4 sky_tranform = scene_data.projection * Mat4(Mat3(scene_data.view));
    p_cmd.pushConstants(skybox_layout, ShaderStageFlagBits::eVertex, 0, sizeof(Mat4), &sky_tranform);

    Viewport viewport {
        .x = 0.0f, .y = 0.0f,
        .width = (float) viewport_size.width, .height = (float) viewport_size.height,
        .minDepth = 0.0f, .maxDepth = 1.0,
    };
    p_cmd.setViewportWithCount(1, &viewport);

    Rect2D scissor {
        .offset = {.x = 0, .y = 0},
        .extent = viewport_size,
    };
    p_cmd.setScissorWithCount(1, &scissor);

    p_cmd.bindShadersEXT(ShaderStageFlagBits::eVertex, skybox_shader.vertex, dispatch_loader);
    p_cmd.bindShadersEXT(ShaderStageFlagBits::eFragment, skybox_shader.fragment, dispatch_loader);
    p_cmd.bindDescriptorSets(PipelineBindPoint::eGraphics, skybox_layout, 0, 1, &draw_image_descriptors, 0, nullptr);

    p_cmd.setRasterizerDiscardEnableEXT(False, dispatch_loader);
    ColorBlendEquationEXT blend_equation {};
    p_cmd.setColorBlendEquationEXT(0, blend_equation, dispatch_loader);
    p_cmd.setRasterizationSamplesEXT(SampleCountFlagBits::e1, dispatch_loader);
    p_cmd.setCullModeEXT(CullModeFlagBits::eNone, dispatch_loader);
    p_cmd.setDepthWriteEnableEXT(False, dispatch_loader);
    p_cmd.setVertexInputEXT({}, {}, dispatch_loader);
    p_cmd.setPrimitiveTopology(PrimitiveTopology::eTriangleList);
    p_cmd.setPrimitiveRestartEnable(False);
    p_cmd.setSampleMaskEXT(SampleCountFlagBits::e1, 1, dispatch_loader);
    p_cmd.setAlphaToCoverageEnableEXT(False, dispatch_loader);
    p_cmd.setPolygonModeEXT(PolygonMode::eFill, dispatch_loader);
    p_cmd.setFrontFace(FrontFace::eCounterClockwise);
    p_cmd.setDepthTestEnable(True);
    p_cmd.setDepthCompareOp(CompareOp::eGreaterOrEqual);
    p_cmd.setDepthBoundsTestEnableEXT(False, dispatch_loader);
    p_cmd.setStencilTestEnable(False);
    p_cmd.setDepthBiasEnableEXT(False, dispatch_loader);

    p_cmd.setLogicOpEnableEXT(False, dispatch_loader);
    p_cmd.setColorBlendEnableEXT(0, {False}, dispatch_loader);
    p_cmd.setColorWriteMaskEXT(0, { ColorComponentFlagBits::eR | ColorComponentFlagBits::eG | ColorComponentFlagBits::eB | ColorComponentFlagBits::eA }, dispatch_loader);

    p_cmd.draw(36, 1, 0, 0);
    p_cmd.endRendering();
}


void Renderer::draw_geometry(CommandBuffer p_cmd) {
    gStats.drawcall_count = 0;
    gStats.triangle_count = 0;
    auto start_time = std::chrono::system_clock::now();

    std::vector<uint32_t> opaque_draws;
    opaque_draws.reserve(draw_context.opaque_surfaces.size());

    for (uint32_t i = 0; i < draw_context.opaque_surfaces.size(); i++) {
        opaque_draws.push_back(i);
    }

    std::sort(opaque_draws.begin(), opaque_draws.end(),
        [&](const auto& index_a, const auto& index_b) {
            const RenderObject& a = draw_context.opaque_surfaces[index_a];
            const RenderObject& b = draw_context.opaque_surfaces[index_b];
            if (a.material == b.material) {
                return a.index_buffer < b.index_buffer;
            } else {
                return a.material < b.material;
            }
        }
    );

    AllocatedBuffer scene_data_buffer = create_buffer(sizeof(GPUSceneData), BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    get_current_frame().deletion_queue.push_function([=, this]() {
        destroy_buffer(scene_data_buffer);
    });

    GPUSceneData* scene_uniform_data = (GPUSceneData*) scene_data_buffer.info.pMappedData;
    *scene_uniform_data = scene_data;
    DescriptorSet global_descriptor = get_current_frame().descriptors.allocate(device, scene_data_descriptor_layout);
    DescriptorWriter writer {};
    writer.write_buffer(0, scene_data_buffer.buffer, sizeof(GPUSceneData), 0, DescriptorType::eUniformBuffer);
    writer.update_set(device, global_descriptor);

    RenderingAttachmentInfo color_attachment {
        .imageView   = draw_image.image_view,
        .imageLayout = ImageLayout::eColorAttachmentOptimal,
        .loadOp      = AttachmentLoadOp::eLoad,
        .storeOp     = AttachmentStoreOp::eStore,
    };
    RenderingAttachmentInfo depth_attachment {
        .imageView   = depth_image.image_view,
        .imageLayout = ImageLayout::eDepthAttachmentOptimal,
        .loadOp      = AttachmentLoadOp::eClear,
        .storeOp     = AttachmentStoreOp::eStore,
        .clearValue  = {
            .depthStencil = {.depth = 0.0f},
        },
    };
    RenderingInfo render_info {
        .renderArea = Rect2D { {0, 0}, {draw_image.image_extent.width, draw_image.image_extent.height} },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
    };

    p_cmd.beginRendering(render_info);

    Viewport viewport {
        .x = 0.0f, .y = 0.0f,
        .width = (float) viewport_size.width, .height = (float) viewport_size.height,
        .minDepth = 0.0f, .maxDepth = 1.0,
    };
    p_cmd.setViewport(0, viewport);

    Rect2D scissor {
        .offset = {.x = 0, .y = 0},
        .extent = viewport_size,
    };
    p_cmd.setScissor(0, scissor);

    MaterialPipeline* previous_pipeline = nullptr;
    MaterialInstance* previous_material = nullptr;
    Buffer previous_index_buffer = VK_NULL_HANDLE;

    auto draw_object = [&](const RenderObject& render_object) {
        if (render_object.material != previous_material) {
            previous_material = render_object.material;

            if (render_object.material->pipeline != previous_pipeline) {
                p_cmd.bindPipeline(PipelineBindPoint::eGraphics, render_object.material->pipeline->pipeline);
                p_cmd.bindDescriptorSets(PipelineBindPoint::eGraphics, render_object.material->pipeline->layout, 0, 1, &global_descriptor, 0, nullptr);
            }

            p_cmd.bindDescriptorSets(PipelineBindPoint::eGraphics, render_object.material->pipeline->layout, 1, 1, &render_object.material->material_set, 0, nullptr);
        }

        if (render_object.index_buffer != previous_index_buffer) {
            previous_index_buffer = render_object.index_buffer;
            p_cmd.bindIndexBuffer(render_object.index_buffer, 0, IndexType::eUint32);
        }
        
        GPUDrawPushConstants push_constants;
        push_constants.vertex_buffer_address = render_object.vertex_buffer_address;
        push_constants.model_matrix = render_object.transform;
        push_constants.flags = show_normals;
        p_cmd.pushConstants(render_object.material->pipeline->layout, ShaderStageFlagBits::eVertex | ShaderStageFlagBits::eFragment, 0, sizeof(GPUDrawPushConstants), &push_constants);

        p_cmd.drawIndexed(render_object.index_count, 1, render_object.first_index, 0, 0);

        gStats.drawcall_count++;
        gStats.triangle_count += render_object.index_count / 3;
    };

    for (const auto& render_object_index : opaque_draws) {
        draw_object(draw_context.opaque_surfaces[render_object_index]);
    }

    for (const RenderObject& render_object : draw_context.transparent_surfaces) {
        draw_object(render_object);
    }

    p_cmd.endRendering();

    auto end_time = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    gStats.mesh_draw_time = elapsed.count() / 1000.0f;
}


void Renderer::draw_imgui(CommandBuffer p_cmd, ImageView p_target_image_view) {
    RenderingAttachmentInfo color_attachment {
        .imageView   = p_target_image_view,
        .imageLayout = ImageLayout::eColorAttachmentOptimal,
        .loadOp      = AttachmentLoadOp::eLoad,
        .storeOp     = AttachmentStoreOp::eStore,
    };
    RenderingInfo render_info {
        .renderArea = Rect2D { Offset2D {0, 0}, viewport_size },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
    };
    p_cmd.beginRendering(render_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), p_cmd);
    p_cmd.endRendering();
}

void Renderer::draw() {

    update_scene();

    device.waitForFences(1, &get_current_frame().render_fence, True, UINT64_MAX);
    get_current_frame().deletion_queue.flush();
    get_current_frame().descriptors.clear_pools(device);

    Result acquisition_result;
    uint32_t swapchain_image_index;
    std::tie(acquisition_result, swapchain_image_index) = device.acquireNextImageKHR(swapchain, UINT64_MAX, get_current_frame().swapchain_semaphore);
    if (acquisition_result == Result::eErrorOutOfDateKHR) {
        resize_requested = true;
        return;
    }
    
    device.resetFences(1, &get_current_frame().render_fence);
    
    CommandBuffer cmd = get_current_frame().command_buffer;
    cmd.reset();

    draw_extent.width  = std::min(viewport_size.width, draw_image.image_extent.width);
    draw_extent.height = std::min(viewport_size.height, draw_image.image_extent.height);

    CommandBufferBeginInfo begin_info {
        .flags = CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    cmd.begin(begin_info);

    transition_image(cmd, draw_image.image, ImageLayout::eUndefined, ImageLayout::eColorAttachmentOptimal);
    transition_image(cmd, depth_image.image, ImageLayout::eUndefined, ImageLayout::eDepthAttachmentOptimal);
    
    draw_geometry(cmd);
    draw_skybox(cmd);

    transition_image(cmd, draw_image.image, ImageLayout::eColorAttachmentOptimal, ImageLayout::eTransferSrcOptimal);
    transition_image(cmd, swapchain_images[swapchain_image_index], ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal);
    
    // Copy draw image to swapchain
    copy_image_to_image(cmd, draw_image.image, swapchain_images[swapchain_image_index], draw_extent, viewport_size);
    transition_image(cmd, swapchain_images[swapchain_image_index], ImageLayout::eTransferDstOptimal, ImageLayout::eColorAttachmentOptimal);
    draw_imgui(cmd, swapchain_image_views[swapchain_image_index]);
    transition_image(cmd, swapchain_images[swapchain_image_index], ImageLayout::eColorAttachmentOptimal, ImageLayout::ePresentSrcKHR);
    
    cmd.end();

    CommandBufferSubmitInfo command_info {.commandBuffer = cmd};
    SemaphoreSubmitInfo wait_info {
        .semaphore = get_current_frame().swapchain_semaphore,
        .value = 1,
        .stageMask = PipelineStageFlagBits2::eColorAttachmentOutput,
        .deviceIndex = 0,
    };
    SemaphoreSubmitInfo signal_info {
        .semaphore = get_current_frame().render_semaphore,
        .value = 1,
        .stageMask = PipelineStageFlagBits2::eAllGraphics,
        .deviceIndex = 0,
    };
    SubmitInfo2 submit {
        .waitSemaphoreInfoCount     = 1,
        .pWaitSemaphoreInfos        = &wait_info,
        .commandBufferInfoCount     = 1,
        .pCommandBufferInfos        = &command_info,
        .signalSemaphoreInfoCount   = 1,
        .pSignalSemaphoreInfos      = &signal_info,
    };

    graphics_queue.submit2(1, &submit, get_current_frame().render_fence);

    PresentInfoKHR present_info {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &get_current_frame().render_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &swapchain_image_index,
    };

    current_frame++;

    Result presentation_result = graphics_queue.presentKHR(present_info);
    if (presentation_result == Result::eErrorOutOfDateKHR || presentation_result == Result::eSuboptimalKHR) {
        resize_requested = true;
    }
}

void Renderer::update_scene() {
    auto start_time = std::chrono::system_clock::now();

    draw_context.opaque_surfaces.clear();
    draw_context.transparent_surfaces.clear();

    for (auto& [_, scene] : loaded_scenes) {
        scene->draw(Mat4(1.0f), draw_context);
    }

    scene_data.view = gCamera.get_view_matrix();
    scene_data.projection = glm::perspective(glm::radians(70.0f), (float)draw_extent.width / (float)draw_extent.height, 1000.0f, 0.1f);
    scene_data.projection[1][1] *= -1.0f;

    scene_data.view_projection = scene_data.projection * scene_data.view;

    scene_data.ambient_color = Vec4(0.1f, 0.1f, 0.1f, 0.0f);
    scene_data.sunlight_color = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
    scene_data.sunlight_direction = Vec4(glm::normalize(Vec3(0.5, 0.5, 0.5)), 1.0f);

    auto end_time = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    gStats.scene_update_time = elapsed.count() / 1000.0f;
}


// --- MaterialMetallicRoughness ---

void MaterialMetallicRoughness::build_pipelines(Renderer* p_renderer) {
    ShaderModule mesh_shader;
    if (!p_renderer->create_shader_module(mesh_spv, mesh_spv_sizeInBytes, mesh_shader)) {
        print("Could not create shader module!");
        return;
    }

    PushConstantRange matrix_range {
        .stageFlags = ShaderStageFlagBits::eVertex | ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants),
    };

    DescriptorLayoutBuilder layout_builder;
    layout_builder.add_binding(0, DescriptorType::eUniformBuffer);
    layout_builder.add_binding(1, DescriptorType::eCombinedImageSampler);
    layout_builder.add_binding(2, DescriptorType::eCombinedImageSampler);

    material_layout = layout_builder.build(p_renderer->device, ShaderStageFlagBits::eVertex | ShaderStageFlagBits::eFragment); // 
    DescriptorSetLayout layouts[] = {
        p_renderer->scene_data_descriptor_layout,
        material_layout
    };

    PipelineLayoutCreateInfo mesh_layout_info {
        .setLayoutCount = 2,
        .pSetLayouts = layouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &matrix_range,
    };

    Result result;
    PipelineLayout new_layout;
    std::tie(result, new_layout) = p_renderer->device.createPipelineLayout(mesh_layout_info);
    if (result != Result::eSuccess) {
        print("Could not create pipeline layout!");
    }

    opaque_pipeline.layout = new_layout;
    transparent_pipeline.layout = new_layout;

    PipelineBuilder pipeline_builder {};
    pipeline_builder
        .set_shaders(mesh_shader, mesh_shader)
        .set_input_topology(PrimitiveTopology::eTriangleList)
        .set_polygon_mode(PolygonMode::eFill)
        .set_cull_mode(CullModeFlagBits::eNone, FrontFace::eCounterClockwise)
        .set_multisampling_none()
        .disable_blending()
        .enable_depth_testing(True, CompareOp::eGreaterOrEqual)
        .set_color_attachment_format(p_renderer->draw_image.image_format)
        .set_depth_format(p_renderer->depth_image.image_format);
    
    pipeline_builder.pipeline_layout = new_layout;
    opaque_pipeline.pipeline = pipeline_builder.build(p_renderer->device);
    
    pipeline_builder
        .enable_blending_additive()
        .enable_depth_testing(False, CompareOp::eGreaterOrEqual);
    transparent_pipeline.pipeline = pipeline_builder.build(p_renderer->device);

    p_renderer->device.destroyShaderModule(mesh_shader);
}

MaterialInstance MaterialMetallicRoughness::write_material(Device p_device, MaterialPass p_pass, const MaterialResources& p_resources, DescriptorAllocatorGrowable& p_descriptor_allocator) {
    MaterialInstance material_data;
    material_data.pass_type = p_pass;
    if (p_pass == MaterialPass::Transparent) {
        material_data.pipeline = &transparent_pipeline;
    } else {
        material_data.pipeline = &opaque_pipeline;
    }

    material_data.material_set = p_descriptor_allocator.allocate(p_device, material_layout);

    writer.clear();
    writer.write_buffer(0, p_resources.data_buffer, sizeof(MaterialConstants), p_resources.data_buffer_offset, DescriptorType::eUniformBuffer);
    writer.write_image(1, p_resources.albedo_image.image_view,          p_resources.albedo_sampler,          ImageLayout::eShaderReadOnlyOptimal, DescriptorType::eCombinedImageSampler);
    writer.write_image(2, p_resources.metal_roughness_image.image_view, p_resources.metal_roughness_sampler, ImageLayout::eShaderReadOnlyOptimal, DescriptorType::eCombinedImageSampler);
    writer.update_set(p_device, material_data.material_set);

    return material_data;
}

void MaterialMetallicRoughness::clear_resources(Device p_device) {
    p_device.destroyDescriptorSetLayout(material_layout);
    p_device.destroyPipelineLayout(opaque_pipeline.layout);
    p_device.destroyPipeline(opaque_pipeline.pipeline);
    p_device.destroyPipeline(transparent_pipeline.pipeline);
}