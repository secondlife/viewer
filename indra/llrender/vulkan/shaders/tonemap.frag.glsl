#version 450
#extension GL_GOOGLE_include_directive : require

#define LL_VULKAN_SHADER 1

layout(location = 0) in vec2 vary_fragcoord;
layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D exposureMap;

layout(push_constant, std430) uniform TonemapParameters
{
    layout(offset = 0) float exposure;
    layout(offset = 4) float tonemap_mix;
    layout(offset = 8) int tonemap_type;
    layout(offset = 12) float gamma;
};

#include "../../../newview/app_settings/shaders/class1/environment/srgbF.glsl"
#include "../../../newview/app_settings/shaders/class1/deferred/tonemapUtilF.glsl"

#if defined(LL_TONEMAP_LEGACY_GAMMA) && !defined(LL_TONEMAP_GAMMA_CORRECT)
#error "LL_TONEMAP_LEGACY_GAMMA requires LL_TONEMAP_GAMMA_CORRECT"
#endif

#ifdef LL_TONEMAP_NO_POST
#define NO_POST 1
#endif

#ifdef LL_TONEMAP_GAMMA_CORRECT
#define GAMMA_CORRECT 1
#endif

#ifdef LL_TONEMAP_LEGACY_GAMMA
#define LEGACY_GAMMA 1
#endif

#include "../../../newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl"
