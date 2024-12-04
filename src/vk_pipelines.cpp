#include "vk_pipelines.h"

#include "util.h"


void PipelineBuilder::clear() {
    shader_stages.clear();
}

Pipeline PipelineBuilder::build(Device p_device) {
    PipelineViewportStateCreateInfo viewport_state {
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    PipelineColorBlendStateCreateInfo color_blending {
        .logicOpEnable   = False,
        .logicOp         = LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment,
    };

    PipelineVertexInputStateCreateInfo vertex_input_info {};

    DynamicState state[] = { DynamicState::eViewport, DynamicState::eScissor };
    PipelineDynamicStateCreateInfo dynamic_info {
        .dynamicStateCount = 2,
        .pDynamicStates    = &state[0],
    };

    GraphicsPipelineCreateInfo pipeline_info {
        .pNext               = &render_info,
        .stageCount          = (uint32_t) shader_stages.size(),
        .pStages             = shader_stages.data(),
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depth_stencil,
        .pColorBlendState    = &color_blending,
        .pDynamicState       = &dynamic_info,
        .layout              = pipeline_layout
    };

    Result result;
    std::vector<Pipeline> new_pipelines;
    std::tie(result, new_pipelines) = p_device.createGraphicsPipelines(VK_NULL_HANDLE, pipeline_info);

    if (result == Result::eSuccess) {
        return new_pipelines[0];
    } else {
        print("Could not create pipeline!");
        return VK_NULL_HANDLE;
    }
}

PipelineBuilder& PipelineBuilder::set_shaders(ShaderModule p_vertex_shader, ShaderModule p_fragment_shader) {
    shader_stages.clear();
    shader_stages.emplace_back(PipelineShaderStageCreateInfo {
        .stage  = ShaderStageFlagBits::eVertex,
        .module = p_vertex_shader,
        .pName  = "vertex",
    });
    shader_stages.emplace_back(PipelineShaderStageCreateInfo {
        .stage  = ShaderStageFlagBits::eFragment,
        .module = p_fragment_shader,
        .pName  = "fragment"
    });

    return *this;
}

PipelineBuilder& PipelineBuilder::set_input_topology(PrimitiveTopology p_topology) {
    input_assembly.topology               = p_topology;
    input_assembly.primitiveRestartEnable = False;

    return *this;
}

PipelineBuilder& PipelineBuilder::set_polygon_mode(PolygonMode p_mode) {
    rasterizer.polygonMode = p_mode;
    rasterizer.lineWidth   = 1.0f;

    return *this;
}

PipelineBuilder& PipelineBuilder::set_cull_mode(CullModeFlags p_cull_mode, FrontFace p_front_face) {
    rasterizer.cullMode  = p_cull_mode;
    rasterizer.frontFace = p_front_face;

    return *this;
}

PipelineBuilder& PipelineBuilder::set_multisampling_none() {
    multisampling.sampleShadingEnable   = False;
    multisampling.rasterizationSamples  = SampleCountFlagBits::e1;
    multisampling.minSampleShading      = 1.0f;
    multisampling.pSampleMask           = nullptr;
    multisampling.alphaToCoverageEnable = False;
    multisampling.alphaToOneEnable      = False;

    return *this;
}

PipelineBuilder& PipelineBuilder::disable_blending() {
    color_blend_attachment.colorWriteMask = ColorComponentFlagBits::eR | ColorComponentFlagBits::eG | ColorComponentFlagBits::eB | ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable    = False;

    return *this;
}

PipelineBuilder& PipelineBuilder::set_color_attachment_format(Format p_format) {
    color_attachment_format = p_format;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachmentFormats = &color_attachment_format;

    return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_format(Format p_format) {
    render_info.depthAttachmentFormat = p_format;

    return *this;
}

PipelineBuilder& PipelineBuilder::disable_depth_testing() {
    depth_stencil.depthTestEnable = False;
    depth_stencil.depthWriteEnable = False;
    depth_stencil.depthCompareOp = CompareOp::eNever;
    depth_stencil.stencilTestEnable = False;
    //depth_stencil.front = {};
   // depth_stencil.back = {};
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;

    return *this;
}

PipelineBuilder& PipelineBuilder::enable_depth_testing(Bool32 p_depth_write_enabled, CompareOp p_compare_op) {
    depth_stencil.depthTestEnable = True;
    depth_stencil.depthWriteEnable = p_depth_write_enabled;
    depth_stencil.depthCompareOp = p_compare_op;
    depth_stencil.stencilTestEnable = False;
    //depth_stencil.front = {};
    //depth_stencil.back = {};
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;

    return *this;
}

PipelineBuilder& PipelineBuilder::enable_blending_additive() {
    color_blend_attachment.colorWriteMask = ColorComponentFlagBits::eR | ColorComponentFlagBits::eG | ColorComponentFlagBits::eB | ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = True;
    color_blend_attachment.srcColorBlendFactor = BlendFactor::eSrcAlpha;
    color_blend_attachment.dstColorBlendFactor = BlendFactor::eOne;
    color_blend_attachment.colorBlendOp = BlendOp::eAdd;
    color_blend_attachment.srcAlphaBlendFactor = BlendFactor::eOne;
    color_blend_attachment.dstAlphaBlendFactor = BlendFactor::eZero;
    color_blend_attachment.alphaBlendOp = BlendOp::eAdd;

    return *this;
}

PipelineBuilder& PipelineBuilder::enable_blending_alpha_blend() {
    color_blend_attachment.colorWriteMask = ColorComponentFlagBits::eR | ColorComponentFlagBits::eG | ColorComponentFlagBits::eB | ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = True;
    color_blend_attachment.srcColorBlendFactor = BlendFactor::eSrcAlpha;
    color_blend_attachment.dstColorBlendFactor = BlendFactor::eOneMinusSrcAlpha;
    color_blend_attachment.colorBlendOp = BlendOp::eAdd;
    color_blend_attachment.srcAlphaBlendFactor = BlendFactor::eOne;
    color_blend_attachment.dstAlphaBlendFactor = BlendFactor::eZero;
    color_blend_attachment.alphaBlendOp = BlendOp::eAdd;

    return *this;
}