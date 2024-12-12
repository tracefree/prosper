#include "input_structures.slang"

uniform SceneData scene_data;

struct LightingParameters {
    Texture2DMS<float> depth_image;
    Texture2DMS<float4> color_image;
    Texture2DMS<float4> normal_image;
    RWTexture2D<float4> storage_image;
};

ParameterBlock<LightingParameters> lighting_params;

func calculate_illumination(float3 albedo, float3 normal)->float4 {
    let light_value = max(dot(normal, scene_data.sunlight_direction.xyz), 0.0);
    let ambient = albedo * scene_data.ambient_color.rgb;
    return float4(albedo * light_value * scene_data.sunlight_color.w + ambient, 1.0);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void compute(
    uint3 thread_id: SV_DispatchThreadID,
) {
    uint2 texel_coord = thread_id.xy;
    uint2 size;
    uint sample_count;
    lighting_params.color_image.GetDimensions(size.x, size.y, sample_count);

    float2 uv = float2(texel_coord) / float2(size);

    // Currently lighting is calculated for each subsample, defeating the purpose of MSAA.
    // TODO: Detect complex pixels and supersample only those.
    if (texel_coord.x < size.x && texel_coord.y < size.y) {
        float4 color = float4(0.0);
        for (int sample_id = 0; sample_id < sample_count; sample_id++) {
            let albedo = lighting_params.color_image.Load(texel_coord, sample_id).rgb;
            let normal = lighting_params.normal_image.Load(texel_coord, sample_id).xyz;
            color += calculate_illumination(albedo, normal);
        }
        color /= float(sample_count);
        lighting_params.storage_image[texel_coord] = color;
    }
}