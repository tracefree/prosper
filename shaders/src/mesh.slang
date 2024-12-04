#include "input_structures.slang"

struct Vertex {
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 color;
}

struct VSOutput {
    float4 PositionCS : SV_Position;
    float4 Normal : VertexNormal;
    float4 Color : VertexColor;
    float2 UV : VertexUV;
};

struct Push {
    float4x4 model_matrix;
    Vertex *vertices;
    int flags;
}

uniform SceneData scene_data;

struct FragmentParams {
    MaterialMetalRoughness material_data;
    Sampler2D albedo_texture;
    Sampler2D metal_roughness_texture;
}

ParameterBlock<FragmentParams> fragment_params;


[[vk::push_constant]]
ConstantBuffer<Push> push_constants;


[shader("vertex")]
VSOutput vertex(uint vertexID: SV_VertexID) {
    let vertex = push_constants.vertices[vertexID];
    let world_position = mul(push_constants.model_matrix, float4(vertex.position, 1.0));
    let world_normal = mul(push_constants.model_matrix, float4(vertex.normal, 0.0));
    return VSOutput(
        mul(scene_data.view_projection, world_position),
        float4(vertex.normal, 0.0),
        vertex.color,
        float2(vertex.uv_x, vertex.uv_y)
    );
}

[shader("fragment")]
float4 fragment(
    float4 position_cs: SV_Position,
    float4 normal: VertexNormal,
    float4 color: VertexColor,
    float2 uv: VertexUV,
) : SV_Target
{
    if (push_constants.flags > 0) {
        return float4(normal.rgb, 1.0);
    } else {
        let light_value = max(dot(normal.xyz, scene_data.sunlight_direction.xyz), 0.0);
        let albedo = color.rgb * fragment_params.albedo_texture.Sample(uv).rgb;
        let ambient = albedo * scene_data.ambient_color.rgb;
        return float4(albedo * light_value * scene_data.sunlight_color.w + ambient, 1.0);
    }
}