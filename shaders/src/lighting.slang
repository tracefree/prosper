#include "input_structures.slang"
#include "common/render_flags.h"

uniform SceneLightsData scene_data;

struct LightingParameters {
    Texture2DMS<float> depth_image;
    Texture2DMS<float4> color_image;
    Texture2DMS<float4> normal_image;
    RWTexture2D<float4> storage_image;
    RWTexture2D<float1> resolved_depth_image;
};

ParameterBlock<LightingParameters> gbuffers;

struct Push {
    uint flags = 0;
    float white_point = 1.0;
    uint viewport_width;
    uint viewport_height;
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

func sum_of_bits(uint i)->uint {
    uint sum = 0;
    for (uint bit = 0; bit < 8; bit++) {
        if ((i & (1 << bit)) > 0) {
            sum += 1;
        }
    }
    return sum;
    /*
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
*/
}

[shader("compute")]
[numthreads(16, 16, 1)]
void compute(
    uint3 thread_id: SV_DispatchThreadID,
) {
    uint2 texel_coord = thread_id.xy;
    uint2 size;
    uint sample_count;
    gbuffers.color_image.GetDimensions(size.x, size.y, sample_count);

    // TODO: Optimize supersampling for complex pixels, reduce branching
    if (texel_coord.x < push_constants.viewport_width && texel_coord.y < push_constants.viewport_height) {
        float2 screen_uv = float2(texel_coord) / float2(push_constants.viewport_width, push_constants.viewport_height);
        uint shaded_mask = 0;
        float4 final_color = float4(0.0, 0.0, 0.0, 1.0);
        float1 final_depth = 0.0;

        bool complex_pixel = false;

        //uint masks[8];
        for (uint id = 0; id < sample_count; id++) {
            //    var mask = uint(gbuffers.normal_image.Load(texel_coord, id).a * 15.0);
            //    masks[id] = mask;
            complex_pixel = complex_pixel || gbuffers.normal_image.Load(texel_coord, id).a > 0.9;
        }
        /*
        for (uint id = 0; id < sample_count; id++) {
            var mask = masks[id];
            for (uint other_id = 0; other_id < sample_count; other_id++) {
                let bit = 1 << other_id;
                if ((bit & mask) > 0 && other_id != id) {
                    if (masks[other_id] != mask) {
                        mask = mask & ~bit;
                    }
                }
            }
            masks[id] = mask;
        }
            */
        //uint total_coverage_bits = 0;
        //uint frag_count = 0;
        for (uint sample_id = 0; sample_id < sample_count; ++sample_id) {
         /*   
            if (((1 << sample_id) & (~shaded_mask)) == 0) {
                continue;
            }
*/
            let normal_roughness_sample = gbuffers.normal_image.Load(texel_coord, sample_id);
            //let coverage = masks[sample_id];
            //let coverage = normal_roughness_sample.a; // uint(normal_roughness_sample.a * 255.0);
            
            //shaded_mask = shaded_mask | coverage;
            //total_coverage_bits += sum_of_bits(coverage);
            //frag_count += 1;

            let albedo_metallic_sample = gbuffers.color_image.Load(texel_coord, sample_id);
            let albedo = albedo_metallic_sample.rgb;
            let metallic = albedo_metallic_sample.a;

            let roughness = 1.0; // normal_roughness_sample.a; // TODO: Reintroduce roughness after optimizing G-buffer
            let normal_ws = normal_roughness_sample.xyz * 2.0 - 1.0;
            let normal_vs = normalize(mul(scene_data.view, float4(normal_ws, 0.0)).xyz);

            let depth = gbuffers.depth_image.Load(texel_coord, sample_id);

            float4 position_nonlinear = mul(scene_data.inverse_projection, float4(screen_uv * 2.0 - 1.0, depth, 1.0));
            float3 position_vs = position_nonlinear.xyz / position_nonlinear.w;

            float3 color = float3(0.0);
            if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_NORMALS) > 0) {
                color += normal_ws * 0.5 + 0.5;
            } else if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_METAL) > 0) {
                color += float3(metallic);
            } else if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_ROUGHNESS) > 0) {
                color += float3(roughness);
            } else {
                color += caluclate_point_light_illumination(albedo.rgb, normal_vs, position_vs, metallic, roughness);
                color += albedo.rgb * scene_data.ambient_color.rgb * scene_data.ambient_color.w;
            }

            //color *= float(sum_of_bits(coverage));
            //color /= sample_count;

            final_color.rgb += color * (complex_pixel ? (1.0 / sample_count) : 1.0);
            final_depth.r += depth * (complex_pixel ? (1.0 / sample_count) : 1.0);
            if (!complex_pixel) {
                break;
            }
        };

        final_color.rgb = tonemap_reinhard(final_color.rgb, push_constants.white_point);

        if ((push_constants.flags & RENDER_FLAG_BIT_SHOW_COMPLEX_PIXELS) > 0 && complex_pixel) {
            final_color.r = 1.0;
        }

        gbuffers.storage_image[texel_coord] = final_color;
        gbuffers.resolved_depth_image[texel_coord] = final_depth;
    }
}