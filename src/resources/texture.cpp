#include <resources/texture.h>
#include <rendering/renderer.h>
#include <rendering/types.h>
#include <stb_image.h>
#include <core/resource_manager.h>

extern Renderer gRenderer;

template<>
void Resource<Texture>::unload() {
    gRenderer.destroy_image(*pointer->image);
};

template<>
Ref<Resource<Texture>> ResourceManager::load<Texture, vk::Format>(const char* p_guid, vk::Format p_format) {
    auto file_path = std::filesystem::path(p_guid);
    AllocatedImage new_image {};
    if (file_path.extension() == ".ktx2") {
        ktxTexture2* k_texture;
        auto result = ktxTexture2_CreateFromNamedFile(file_path.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &k_texture);
        if (result != KTX_SUCCESS) {
            print("Could not create KTX texture!");
        }

        auto color_model = ktxTexture2_GetColorModel_e(k_texture);
        ktx_transcode_fmt_e texture_format;
        PhysicalDeviceFeatures features = gRenderer.physical_device.getFeatures();
        if (color_model != KHR_DF_MODEL_RGBSDA) {
            if (color_model == KHR_DF_MODEL_UASTC && features.textureCompressionASTC_LDR) {
                texture_format = KTX_TTF_ASTC_4x4_RGBA;
            } else if (color_model == KHR_DF_MODEL_ETC1S && features.textureCompressionETC2) {
                texture_format = KTX_TTF_ETC;
            } else if (features.textureCompressionASTC_LDR) {
                texture_format = KTX_TTF_ASTC_4x4_RGBA;
            } else if (features.textureCompressionETC2) {
                texture_format = KTX_TTF_ETC2_RGBA;
            } else if (features.textureCompressionBC) {
                texture_format = KTX_TTF_BC3_RGBA;
            } else {
                print("Could not transcode texture!");
            }
            result = ktxTexture2_TranscodeBasis(k_texture, KTX_TTF_BC7_RGBA, 0);
            if (result != KTX_SUCCESS) {
                print("Could not transcode texture!");
            }
        }
        
        ktxVulkanTexture texture;
        result = ktxTexture2_VkUploadEx(k_texture, &gRenderer.ktx_device_info, &texture, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if (result != KTX_SUCCESS) {
            print("Could not upload KTX texture!");
        }
        new_image.ktx_texture = texture;
        new_image.image = texture.image;

        ImageViewCreateInfo view_info {
            .image      = texture.image,
            .viewType   = static_cast<ImageViewType>(texture.viewType),
            .format     = static_cast<Format>(texture.imageFormat),
            .subresourceRange = ImageSubresourceRange {
                .aspectMask     = ImageAspectFlagBits::eColor,
                .baseMipLevel   = 0,
                .levelCount     = texture.levelCount,
                .baseArrayLayer = 0,
                .layerCount     = texture.layerCount,
            }
        };
        Result view_result;
        std::tie(view_result, new_image.image_view) = gRenderer.device.createImageView(view_info);
        ktxTexture2_Destroy(k_texture);
    } else {
        int w, h, nrChannels;
        unsigned char* data = stbi_load(file_path.c_str(), &w, &h, &nrChannels, 4);
        new_image = gRenderer.create_image(data, Extent3D {uint32_t(w), uint32_t(h), 1}, p_format, ImageUsageFlagBits::eSampled, true);
    }
    
    auto& resource = (*ResourceManager::get<Texture>(p_guid));
    resource->image = std::make_unique<AllocatedImage>(new_image);
    resource.set_load_status(LoadStatus::LOADED);

    return ResourceManager::get<Texture>(p_guid);
}