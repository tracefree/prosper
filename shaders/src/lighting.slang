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

func caluclate_point_light_illumination(float3 albedo, float3 normal_vs, float3 position_vs, float metallic, float roughness)->float3 {
    var diffuse = float3(0.0);
    var specular = float3(0.0);
    for (int light_index = 0; light_index < 8; ++light_index) {
        let point_light = scene_data.point_lights[light_index];
        let light_positon_vs = mul(scene_data.view, float4(point_light.position, 1.0)).xyz;
        let light_vector = light_positon_vs - position_vs;
        let light_direction = normalize(light_vector);
        let light_value = max(dot(normal_vs, light_direction), 0.0);
        let attenuation = 1.0 / dot(light_vector, light_vector);
        let reflect_direction = reflect(-light_direction, normal_vs);

        diffuse += albedo * light_value * point_light.color * point_light.intensity * attenuation * roughness;

        let view_direction = normalize(-position_vs);
        let shininess = metallic;
        specular += albedo * point_light.color * pow(max(dot(view_direction, normalize(reflect_direction)), 0.0), 32.0) * metallic * attenuation;
    }
    return diffuse + specular;
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

    // TODO: Optimize supersampling for complex pixels, reduce branching
    if (texel_coord.x < size.x && texel_coord.y < size.y) {
        float4 color = float4(0.0, 0.0, 0.0, 1.0);
        
        bool complex_pixel = false;
        for (int id = 0; id < sample_count; id++) {
            complex_pixel = complex_pixel || gbuffers.normal_image.Load(texel_coord, id).a > 0.0;
        }
        
        int sample_id = 0;
        do {
            let albedo_metallic_sample = gbuffers.color_image.Load(texel_coord, sample_id);
            let albedo = albedo_metallic_sample.rgb;
            let metallic = albedo_metallic_sample.a;

            let normal_roughness_sample = gbuffers.normal_image.Load(texel_coord, sample_id);
            let roughness = 1.0; // normal_roughness_sample.a; // TODO: Reintroduce roughness after optimizing G-buffer
            let normal_ws = normal_roughness_sample.xyz;
            let normal_vs = normalize(mul(scene_data.view, float4(normal_ws, 0.0)).xyz);

            let depth = gbuffers.depth_image.Load(texel_coord, sample_id);

            float4 position_nonlinear = mul(scene_data.inverse_projection, float4(screen_uv * 2.0 - 1.0, depth, 1.0));
            float3 position_vs = position_nonlinear.xyz / position_nonlinear.w;

            if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_NORMALS) > 0) {
                color.rgb += normal_vs * 0.5 + 0.5;
            } else if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_METAL) > 0) {
                color.rgb += float3(metallic);
            } else if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_ROUGHNESS) > 0) {
                color.rgb += float3(roughness);
            } else {
                color.rgb += caluclate_point_light_illumination(albedo.rgb, normal_vs, position_vs, metallic, roughness);
                color.rgb += albedo.rgb * scene_data.ambient_color.rgb * scene_data.ambient_color.w;
            }

            if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_COMPLEX_PIXELS) > 0) {
                color.r = complex_pixel ? 1.0 : color.r;
            }

            sample_id++;
        } while ((sample_id < sample_count) && complex_pixel);
        
        color.rgb /= float(sample_id);
        color.rgb = tonemap_reinhard(color.rgb, push_constants.white_point);
        gbuffers.storage_image[texel_coord] = color;
    }
}