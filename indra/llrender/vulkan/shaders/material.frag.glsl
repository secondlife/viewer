#version 450
#extension GL_GOOGLE_include_directive : require

#define LL_VULKAN_SHADER 1
#define DIFFUSE_ALPHA_MODE 0
#define HAS_NORMAL_MAP 1
#define HAS_SPECULAR_MAP 1
#define GBUFFER_FLAG_HAS_ATMOS 0.34

#if defined(LL_VULKAN_MATERIAL_PRODUCTION) && LL_VULKAN_MATERIAL_PRODUCTION == 1
#define HAS_EMISSIVE 1
#define HAS_SUN_SHADOW 1
#define SUN_SHADOW 1
#define SPOT_SHADOW 1
#endif

layout(location = 0) in vec3 vary_position;
layout(location = 1) in vec3 vary_tangent;
layout(location = 2) flat in float vary_sign;
layout(location = 3) in vec3 vary_normal;
layout(location = 4) in vec2 vary_texcoord1;
layout(location = 5) in vec2 vary_texcoord2;
layout(location = 6) in vec4 vertex_color;
layout(location = 7) in vec2 vary_texcoord0;

#if defined(LL_VULKAN_MATERIAL_PRODUCTION) && LL_VULKAN_MATERIAL_PRODUCTION == 1
layout(location = 0) out vec4 frag_data[4];
#else
layout(location = 0) out vec4 frag_data[3];
#endif

layout(set = 1, binding = 0) uniform sampler2D diffuseMap;
layout(set = 1, binding = 1) uniform sampler2D bumpMap;
layout(set = 1, binding = 2) uniform sampler2D specularMap;

layout(std140, set = 0, binding = 0) uniform MaterialParameterPacket
{
    vec4 material_parameter_vectors[17];
};

float materialParameterWord(int index)
{
    return material_parameter_vectors[index / 4][index % 4];
}

vec4 materialParameterVec4(int first)
{
    return vec4(materialParameterWord(first + 0), materialParameterWord(first + 1),
                materialParameterWord(first + 2), materialParameterWord(first + 3));
}

#define specular_color materialParameterVec4(57)
#define clipPlane materialParameterVec4(61)
#define env_intensity materialParameterWord(65)
#define emissive_brightness materialParameterWord(66)
#define mirror_flag materialParameterWord(67)

#include "../../../newview/app_settings/shaders/class1/deferred/globalF.glsl"
#include "../../../newview/app_settings/shaders/class3/deferred/materialF.glsl"
