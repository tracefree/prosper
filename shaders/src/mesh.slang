#include "input_structures.slang"
#include "common/render_flags.h"

struct VSOutput {
    float4 PositionCS : SV_Position;
    float3x3 TBN_WS : TBN_WS;
    float4 Color : VertexColor;
    float2 UV : VertexUV;
};

struct Push {
    float4x4 model_matrix;
    float4x4 view_projection;
    Vertex *vertices;
}

struct FragmentParams {
    MaterialMetalRoughness material_data;
    Sampler2D albedo_texture;
    Sampler2D normal_texture;
    Sampler2D metal_roughness_texture;
}

struct FSOutput {
    float4 Albedo : COLOR0;
    float4 Normal : COLOR1;
};

[[vk::binding(0)]]
ParameterBlock<FragmentParams> fragment_params;


[[vk::push_constant]]
ConstantBuffer<Push> push_constants;


[shader("vertex")]
VSOutput vertex(uint vertexID: SV_VertexID) {
    let vertex = push_constants.vertices[vertexID];
    let model_matrix = push_constants.model_matrix;
    var tbn_ws = float3x3(
        normalize(mul(model_matrix, float4(vertex.tangent.xyz, 0.0)).xyz),
        normalize(mul(model_matrix, float4(cross(vertex.normal, vertex.tangent.xyz) * vertex.tangent.w, 0.0)).xyz),
        normalize(mul(model_matrix, float4(vertex.normal, 0.0)).xyz)
    );

    let world_position = mul(push_constants.model_matrix, float4(vertex.position, 1.0));
    
    return VSOutput(
        mul(push_constants.view_projection, world_position),
        tbn_ws,
        vertex.color,
        float2(vertex.uv_x, vertex.uv_y)
    );
}

[shader("fragment")]
FSOutput fragment(
    float4 position_cs: SV_Position,
    float3x3 tbn_ws: TBN_WS,
    float4 color: VertexColor,
    float2 uv: VertexUV,
    uint coverage: SV_Coverage,
)
{
    let albedo_texture_sample = fragment_params.albedo_texture.Sample(uv);
    if (albedo_texture_sample.a < 0.5) {
        discard;
    }

    let albedo = fragment_params.material_data.albedo_factors.rgb * color.rgb * albedo_texture_sample.rgb;

    let normal_texture_sample = fragment_params.normal_texture.Sample(uv).xyz;
    var normal_ts = normal_texture_sample * 2.0 - 1.0;
    let normal_ws = mul(normal_ts, tbn_ws);

    let metallic_roughness_sample = fragment_params.metal_roughness_texture.Sample(uv).rgb;
    let metallic  = fragment_params.material_data.metal_roughness_factors.r * metallic_roughness_sample.b;
    let roughness = fragment_params.material_data.metal_roughness_factors.r * metallic_roughness_sample.g;

    // TODO: Don't use full float for pixel complexity? Put roughness back into the gbuffer
    let complex_pixel = coverage == 15 ? 0.0 : 1.0;
    return FSOutput(
        float4(albedo, metallic),
        float4(normal_ws * 0.5 + 0.5, complex_pixel)
    );
}