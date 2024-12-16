#include "input_structures.slang"

struct SkinningData {
    uint4 joint_ids;
    float4 weights;
}

struct Push {
    float time;
    uint number_vertices;
    Vertex *input_vertices;
    Vertex *output_vertices;
    SkinningData *skinning_data;
    float4x4 *joint_matrices;
}

[[vk::push_constant]]
ConstantBuffer<Push> push_constants;

[shader("compute")]
[numthreads(256, 1, 1)]
void compute(
    uint3 thread_id: SV_DispatchThreadID
) {
    uint index = thread_id.x;
    if (index >= push_constants.number_vertices) {
        return;
    }

    Vertex in_vertex = push_constants.input_vertices[index];
    push_constants.output_vertices[index] = in_vertex;

    let joint_ids = push_constants.skinning_data[index].joint_ids;
    let weights = push_constants.skinning_data[index].weights;
    let joint_matrices = push_constants.joint_matrices;
    float4x4 skin_matrix =
        (weights.x * joint_matrices[joint_ids.x]) +
        (weights.y * joint_matrices[joint_ids.y]) +
        (weights.z * joint_matrices[joint_ids.z]) +
        (weights.w * joint_matrices[joint_ids.w]);

    push_constants.output_vertices[index].position = mul(skin_matrix, float4(in_vertex.position, 1.0)).xyz;
}