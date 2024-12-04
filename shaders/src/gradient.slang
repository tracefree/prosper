struct Push {
    float4 data1;
    float4 data2;
    float4 _data3;
    float4 _data4;
}

[[vk::push_constant]]
ConstantBuffer<Push> push_constants;

[shader("compute")]
[numthreads(16, 16, 1)]
void compute(uint3 thread_id: SV_DispatchThreadID, uniform RWTexture2D<float4> image) {
    uint2 texel_coord = thread_id.xy;
    float4 top_color = push_constants.data1;
    float4 bottom_color = push_constants.data2;

    uint2 size;
    image.GetDimensions(size.x, size.y);
    
    if (texel_coord.x < size.x && texel_coord.y < size.y) {
        float blend = float(texel_coord.y) / (size.y);
        image[texel_coord] = lerp(top_color, bottom_color, blend);
    }
}