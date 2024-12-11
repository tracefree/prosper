[shader("compute")]
[numthreads(16, 16, 1)]
void compute(
    uint3 thread_id: SV_DispatchThreadID,
    uniform Texture2DMS<float4> color_image,
    uniform RWTexture2D<float4> storage_image
) {
    uint2 texel_coord = thread_id.xy;
    uint2 size;
    uint sample_count;
    color_image.GetDimensions(size.x, size.y, sample_count);

    float2 uv = float2(texel_coord) / float2(size);

    if (texel_coord.x < size.x && texel_coord.y < size.y) {
        float4 color = float4(0.0);
        for (int sample_id = 0; sample_id < sample_count; sample_id++) {
            color += color_image.Load(texel_coord, sample_id);
        }
        color /= float(sample_count);
        storage_image[texel_coord] = color;
    }
}