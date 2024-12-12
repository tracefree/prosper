struct PointLight {
    float3 position;
    float intensity;
    float3 color;
    float _padding1;
}

struct SceneLightsData {
    float4x4 view;
    float4x4 projection;
    float4x4 view_projection;
    float4x4 inverse_projection;
    float4 ambient_color;
    float4 sunlight_direction;
    float4 sunlight_color;
    PointLight point_lights[8];
}

struct MaterialMetalRoughness {
    float4 albedo_factors;
    float4 metal_roughness_factors;
}