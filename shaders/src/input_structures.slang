struct SceneLightsData {
    float4x4 view;
    float4x4 projection;
    float4x4 view_projection;
    float4 ambient_color;
    float4 sunlight_direction;
    float4 sunlight_color;
}

struct MaterialMetalRoughness {
    float4 albedo_factors;
    float4 metal_roughness_factors;
}