struct VSOutput {
    float4 PositionCS : SV_Position;
};

static float3 positions[8] = {
    float3(1.0, 1.0, 1.0),
    float3(1.0, 1.0, -1.0),
    float3(1.0, -1.0, 1.0),
    float3(1.0, -1.0, -1.0),
    float3(-1.0, 1.0, 1.0),
    float3(-1.0, 1.0, -1.0),
    float3(-1.0, -1.0, 1.0),
    float3(-1.0, -1.0, -1.0),
};

static int indices[36] = { 0, 3, 2, 0, 1, 3, 0, 2, 6, 0, 6, 4, 0, 4, 1, 5, 1, 4, 5, 7, 1, 3, 1, 7, 5, 4, 7, 6, 7, 4, 2, 3, 6, 6, 3, 7 };

struct VertexOut {
    float4 position_vs : SV_Position;
    float3 position_world;
}

struct SkyboxParameters {
    SamplerCube<float4> skybox_image;
};

struct Push {
    float4x4 view_projection;
}

[[vk::binding(0)]]
ParameterBlock<SkyboxParameters> skybox_parameters;

[[vk::push_constant]]
ConstantBuffer<Push> push_constants;

[shader("vertex")]
VertexOut vertex(uint vertex_id: SV_VertexID) {
    let position_world = positions[indices[vertex_id]];
    let position_vs = mul(push_constants.view_projection, float4(position_world, 1.0));
    return VertexOut(
        float4(position_vs.xy, 0.0, position_vs.w),
        position_world
    );
}

[shader("fragment")]
float4 fragment(
    float4 position_cs: SV_Position,
    float3 position_world,
) : SV_Target
{
    return skybox_parameters.skybox_image.Sample(position_world);
    /*
    var bottom_color = float4(0.3, 0.2, 0.2, 1.0);
    let top_color = float4(0.3, 0.3, 0.9, 1.0);
    let theta = acos(-position_world.y / length(position_world));
    return lerp(bottom_color, top_color, clamp(theta / 3.141, 0.0, 1.0));
    */
}