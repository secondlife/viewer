#version 450
#extension GL_GOOGLE_include_directive : require

#define LL_VULKAN_SHADER 1
#define DIFFUSE_ALPHA_MODE 0
#define HAS_NORMAL_MAP 1
#define HAS_SPECULAR_MAP 1

#if defined(LL_VULKAN_MATERIAL_PRODUCTION) && LL_VULKAN_MATERIAL_PRODUCTION == 1
#define HAS_EMISSIVE 1
#define HAS_SUN_SHADOW 1
#define SUN_SHADOW 1
#define SPOT_SHADOW 1
#endif

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 3) in vec4 diffuse_color;
layout(location = 4) in vec4 tangent;
layout(location = 5) in vec2 texcoord1;
layout(location = 6) in vec2 texcoord2;

layout(location = 0) out vec3 vary_position;
layout(location = 1) out vec3 vary_tangent;
layout(location = 2) flat out float vary_sign;
layout(location = 3) out vec3 vary_normal;
layout(location = 4) out vec2 vary_texcoord1;
layout(location = 5) out vec2 vary_texcoord2;
layout(location = 6) out vec4 vertex_color;
layout(location = 7) out vec2 vary_texcoord0;

// MaterialParameters is 68 tightly packed floats. vec4[17] keeps those bytes
// contiguous under std140, including the mat3-to-mat4 boundary at word 41.
layout(std140, set = 0, binding = 0) uniform MaterialParameterPacket
{
    vec4 material_parameter_vectors[17];
};

float materialParameterWord(int index)
{
    return material_parameter_vectors[index / 4][index % 4];
}

mat4 materialParameterMat4(int first)
{
    return mat4(
        vec4(materialParameterWord(first + 0), materialParameterWord(first + 1),
             materialParameterWord(first + 2), materialParameterWord(first + 3)),
        vec4(materialParameterWord(first + 4), materialParameterWord(first + 5),
             materialParameterWord(first + 6), materialParameterWord(first + 7)),
        vec4(materialParameterWord(first + 8), materialParameterWord(first + 9),
             materialParameterWord(first + 10), materialParameterWord(first + 11)),
        vec4(materialParameterWord(first + 12), materialParameterWord(first + 13),
             materialParameterWord(first + 14), materialParameterWord(first + 15)));
}

mat3 materialParameterMat3(int first)
{
    return mat3(
        vec3(materialParameterWord(first + 0), materialParameterWord(first + 1), materialParameterWord(first + 2)),
        vec3(materialParameterWord(first + 3), materialParameterWord(first + 4), materialParameterWord(first + 5)),
        vec3(materialParameterWord(first + 6), materialParameterWord(first + 7), materialParameterWord(first + 8)));
}

#define modelview_matrix materialParameterMat4(0)
#define modelview_projection_matrix materialParameterMat4(16)
#define normal_matrix materialParameterMat3(32)
#define texture_matrix0 materialParameterMat4(41)

#define main llMaterialVertexMain
#include "../../../newview/app_settings/shaders/class1/deferred/materialV.glsl"
#undef main

void main()
{
    llMaterialVertexMain();
    gl_Position.z = 0.5 * (gl_Position.z + gl_Position.w);
}
