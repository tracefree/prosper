#pragma once

#include <vector>
#define VULKAN_HPP_ASSERT_ON_RESULT //
#include <vulkan/vulkan.hpp>

using namespace vk;

class PipelineBuilder {
public:
    std::vector<PipelineShaderStageCreateInfo> shader_stages {};
    PipelineInputAssemblyStateCreateInfo input_assembly {};
    PipelineRasterizationStateCreateInfo rasterizer {};
    PipelineColorBlendAttachmentState color_blend_attachment {};
    PipelineMultisampleStateCreateInfo multisampling {};
    PipelineLayout pipeline_layout {};
    PipelineDepthStencilStateCreateInfo depth_stencil {};
    PipelineRenderingCreateInfo render_info {};
    Format color_attachment_format;

    PipelineBuilder() { clear(); }

    void clear();
    PipelineBuilder& set_shaders(ShaderModule p_vertex_shader, ShaderModule p_fragment_shader);
    PipelineBuilder& set_input_topology(PrimitiveTopology p_topology);
    PipelineBuilder& set_polygon_mode(PolygonMode p_mode);
    PipelineBuilder& set_cull_mode(CullModeFlags p_cull_mode, FrontFace p_front_face);
    PipelineBuilder& set_multisampling_none();
    PipelineBuilder& disable_blending();
    PipelineBuilder& set_color_attachment_format(Format p_format);
    PipelineBuilder& set_depth_format(Format p_format);
    PipelineBuilder& disable_depth_testing();
    PipelineBuilder& enable_depth_testing(Bool32 p_depth_write_enabled, CompareOp p_compare_op);
    PipelineBuilder& enable_blending_additive();
    PipelineBuilder& enable_blending_alpha_blend();
    
    Pipeline build(Device p_device);
};