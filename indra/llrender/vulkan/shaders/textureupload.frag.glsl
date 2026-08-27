#version 450
#extension GL_GOOGLE_include_directive : require

#define LL_VULKAN_SHADER 1

layout(location = 0) in vec2 tc;
layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

#include "../../../newview/app_settings/shaders/class1/interface/copyF.glsl"
