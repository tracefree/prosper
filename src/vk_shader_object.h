#pragma once

#include "util.h"

#include "vk_types.h"


struct ShaderObject {
    ShaderEXT vertex;
    ShaderEXT fragment;

    PipelineLayout layout;
    DescriptorSet descriptor_set;

    ShaderObject& set_shaders(ShaderModule p_vertex_shader, ShaderModule p_fragment_shader);
    ShaderObject& set_input_topology(PrimitiveTopology p_topology);
    ShaderObject& set_polygon_mode(PolygonMode p_mode);
    ShaderObject& set_cull_mode(CullModeFlags p_cull_mode, FrontFace p_front_face);
    ShaderObject& set_multisampling_none();
    ShaderObject& disable_blending();
    ShaderObject& set_color_attachment_format(Format p_format);
    ShaderObject& set_depth_format(Format p_format);
    ShaderObject& disable_depth_testing();
    ShaderObject& enable_depth_testing(Bool32 p_depth_write_enabled, CompareOp p_compare_op);
    ShaderObject& enable_blending_additive();
    ShaderObject& enable_blending_alpha_blend();

    void bind(CommandBuffer p_cmd);
};

