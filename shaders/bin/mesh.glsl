#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 52 0
layout(location = 0)
out vec4 entryPointParam_fragment_0;


#line 52
layout(location = 0)
in vec4 normal_vs_0;


#line 52
void main()
{

#line 52
    entryPointParam_fragment_0 = vec4(normal_vs_0.xyz * 0.5 + 0.5, 1.0);

#line 52
    return;
}

