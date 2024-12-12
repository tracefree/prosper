#include "input_structures.slang"
#include "common/render_flags.h"

uniform SceneLightsData scene_data;

struct LightingParameters {
    Texture2DMS<float> depth_image;
    Texture2DMS<float4> color_image;
    Texture2DMS<float4> normal_image;
    RWTexture2D<float4> storage_image;
};

ParameterBlock<LightingParameters> gbuffers;

struct Push {
    uint flags = 0;
    float white_point = 1.0;
}

[[vk::push_constant]]
ConstantBuffer<Push> push_constants;


func caluclate_point_light_illumination(float3 albedo, float3 normal_vs, float3 position_vs)->float3 {
    var diffuse = float3(0.0);
    for (int light_index = 0; light_index < 8; ++light_index) {
        let point_light = scene_data.point_lights[light_index];
        let light_positon_vs = mul(scene_data.view, float4(point_light.position, 1.0)).xyz;
        let light_vector = light_positon_vs - position_vs;
        let light_direction = normalize(light_vector);
        let light_value = max(dot(normal_vs, light_direction), 0.0);
        let attenuation = 1.0 / dot(light_vector, light_vector);
        diffuse += albedo * light_value * point_light.color * point_light.intensity * attenuation;
    }
    return diffuse;
}

func calculate_illumination(float3 albedo, float3 normal) -> float3 {
    let light_value = max(dot(normal, scene_data.sunlight_direction.xyz), 0.0);
    let ambient = albedo * scene_data.ambient_color.rgb;
    return albedo * light_value * scene_data.sunlight_color.w + ambient;
}

func tonemap_reinhard(float3 color, float white_point)->float3 {
    float luminance = dot(float3(0.2126, 0.7152, 0.0722), color);
    float tonemapped_luminance = (1.0 + luminance / (white_point * white_point)) / (1.0 + luminance);
    return color * tonemapped_luminance;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void compute(
    uint3 thread_id: SV_DispatchThreadID,
    uint huh: SV_Coverage
) {
    uint2 texel_coord = thread_id.xy;
    uint2 size;
    uint sample_count;
    gbuffers.color_image.GetDimensions(size.x, size.y, sample_count);

    float2 screen_uv = float2(texel_coord) / float2(size);

    // Currently lighting is calculated for each subsample, defeating the purpose of MSAA.
    // TODO: Detect complex pixels and supersample only those.
    if (texel_coord.x < size.x && texel_coord.y < size.y) {
        float4 color = float4(0.0, 0.0, 0.0, 1.0);
        var is_complex = false;
        var prev_albedo = float3(0.0);
        for (int sample_id = 0; sample_id < sample_count; sample_id++) {
            let albedo = gbuffers.color_image.Load(texel_coord, sample_id);
            let normal_ws = gbuffers.normal_image.Load(texel_coord, sample_id).xyz;
            let normal_vs = normalize(mul(scene_data.view, float4(normal_ws, 0.0)).xyz);
            let depth = gbuffers.depth_image.Load(texel_coord, sample_id);

            float4 position_nonlinear = mul(scene_data.inverse_projection, float4(screen_uv * 2.0 - 1.0, depth, 1.0));
            float3 position_vs = position_nonlinear.xyz / position_nonlinear.w;

            if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_NORMALS) > 0) {
                color.rgb += normal_vs * 0.5 + 0.5;
            } else {
                color.rgb += caluclate_point_light_illumination(albedo.rgb, normal_vs, position_vs);
                color.rgb += albedo.rgb * scene_data.ambient_color.rgb * scene_data.ambient_color.w;
            }
        }
        color.rgb /= float(sample_count);
        color.rgb = tonemap_reinhard(color.rgb, push_constants.white_point);
        gbuffers.storage_image[texel_coord] = color;
    }
}