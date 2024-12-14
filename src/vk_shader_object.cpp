#include "vk_shader_object.h"


ShaderObject& ShaderObject::set_shaders(ShaderModule p_vertex_shader, ShaderModule p_fragment_shader) {
    return *this;
}


ShaderObject& ShaderObject::set_input_topology(PrimitiveTopology p_topology) {
    return *this;
}


ShaderObject& ShaderObject::set_polygon_mode(PolygonMode p_mode) {
    return *this;
}


ShaderObject& ShaderObject::set_cull_mode(CullModeFlags p_cull_mode, FrontFace p_front_face) {
    cull_mode = p_cull_mode;
    front_face = p_front_face;
    return *this;
}


ShaderObject& ShaderObject::set_multisampling_none() {
    return *this;
}


ShaderObject& ShaderObject::disable_blending() {
    return *this;
}


ShaderObject& ShaderObject::set_color_attachment_format(Format p_format) {
    return *this;
}


ShaderObject& ShaderObject::set_depth_format(Format p_format) {
    return *this;
}


ShaderObject& ShaderObject::disable_depth_testing() {
    return *this;
}

ShaderObject& ShaderObject::enable_depth_writing(Bool32 p_enable) {
    depth_write_enabled = p_enable;
    return *this;
}

ShaderObject& ShaderObject::enable_depth_testing(Bool32 p_depth_write_enabled, CompareOp p_compare_op) {
    return *this;
}


ShaderObject& ShaderObject::enable_blending_additive() {
    return *this;
}


ShaderObject& ShaderObject::enable_blending_alpha_blend() {
    return *this;
}

void ShaderObject::bind(CommandBuffer p_cmd) {
    p_cmd.setRasterizerDiscardEnableEXT(False);
    
    p_cmd.setAlphaToCoverageEnableEXT(False);
    p_cmd.setPolygonModeEXT(PolygonMode::eFill);
    p_cmd.setCullModeEXT(cull_mode);
    p_cmd.setFrontFace(front_face);
    p_cmd.setDepthTestEnable(True);
    p_cmd.setDepthWriteEnableEXT(depth_write_enabled);
    p_cmd.setDepthCompareOp(CompareOp::eGreaterOrEqual);
    p_cmd.setDepthBoundsTestEnableEXT(False);
    p_cmd.setDepthBiasEnableEXT(False);
    p_cmd.setStencilTestEnable(False);

    if (vertex != nullptr) {
        p_cmd.bindShadersEXT(ShaderStageFlagBits::eVertex, vertex);
        
        p_cmd.setVertexInputEXT({}, {});
        p_cmd.setPrimitiveTopology(PrimitiveTopology::eTriangleList);
        p_cmd.setPrimitiveRestartEnable(False);
    }

    if (fragment != nullptr) {
        p_cmd.bindShadersEXT(ShaderStageFlagBits::eFragment, fragment);

        p_cmd.setLogicOpEnableEXT(False);        
        p_cmd.setColorBlendEnableEXT(0, {False});
        ColorBlendEquationEXT blend_equation {};
        p_cmd.setColorBlendEquationEXT(0, blend_equation);
        p_cmd.setColorWriteMaskEXT(0, { ColorComponentFlagBits::eR | ColorComponentFlagBits::eG | ColorComponentFlagBits::eB | ColorComponentFlagBits::eA });
    }

    if (compute != nullptr) {
        p_cmd.bindShadersEXT(ShaderStageFlagBits::eCompute, compute);
    }
}

void ShaderObject::cleanup(Device p_device) {
    if (compute != nullptr) {
        p_device.destroyShaderEXT(compute);   
    }
    if (layout != nullptr) {
        p_device.destroyPipelineLayout(layout);
    }
}