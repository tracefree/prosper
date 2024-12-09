#include "input_structures.slang"

struct Vertex {
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
    float4 color;
    float4 tangent;
}

struct VSOutput {
    float4 PositionCS : SV_Position;
    float3x3 TBN_VS : TBN_VS;
    float4 Color : VertexColor;
    float2 UV : VertexUV;
};

struct Push {
    float4x4 model_matrix;
    Vertex *vertices;
}

uniform SceneData scene_data;

struct FragmentParams {
    MaterialMetalRoughness material_data;
    Sampler2D albedo_texture;
    Sampler2D normal_texture;
    Sampler2D metal_roughness_texture;
}

ParameterBlock<FragmentParams> fragment_params;


[[vk::push_constant]]
ConstantBuffer<Push> push_constants;


[shader("vertex")]
VSOutput vertex(uint vertexID: SV_VertexID) {
    let vertex = push_constants.vertices[vertexID];
    
    let model_matrix = push_constants.model_matrix;
    var tbn_vs = float3x3(
        normalize(mul(float4(vertex.tangent.xyz, 0.0), model_matrix).xyz),
        normalize(mul(float4(cross(vertex.normal, vertex.tangent.xyz) * vertex.tangent.w, 0.0), model_matrix).xyz),
        normalize(mul(float4(vertex.normal, 0.0), model_matrix).xyz)
    ); 
    
    let world_position = mul(push_constants.model_matrix, float4(vertex.position, 1.0));
    
    return VSOutput(
        mul(scene_data.view_projection, world_position),
        tbn_vs,
        vertex.color,
        float2(vertex.uv_x, vertex.uv_y)
    );
}

[shader("fragment")]
float4 fragment(
    float4 position_cs : SV_Position,
    float3x3 tbn_ws : TBN_WS,
    float4 color : VertexColor,
    float2 uv : VertexUV,
) : SV_Target
{
    if (-2 > 0) {
        return float4(tbn_ws[2].rgb * 0.5 + 0.5, 1.0);
    } else {
        let albedo_texture_sample = fragment_params.albedo_texture.Sample(uv);
        if (albedo_texture_sample.a < 0.5) {
            discard;
        }

        let normal_texture_sample = fragment_params.normal_texture.Sample(uv).xyz;
        var normal_ts = normal_texture_sample * 2.0 - 1.0;
        let normal_ws = mul(normal_ts, tbn_ws);
        let light_value = max(dot(normal_ws, scene_data.sunlight_direction.xyz), 0.0);
        let albedo = color.rgb * pow(albedo_texture_sample.rgb, 2.2);
        let ambient = albedo * scene_data.ambient_color.rgb;

        let metallic = fragment_params.metal_roughness_texture.Sample(uv).r;
        //return float4(float3(metallic), 1.0);
        return float4(albedo * light_value * scene_data.sunlight_color.w + ambient, albedo_texture_sample.a);
    }
}