#version 450
#extension GL_GOOGLE_include_directive : require

#define LL_VULKAN_SHADER 1

layout(location = 0) in vec3 position;
layout(location = 0) out vec2 tc;

#include "../../../newview/app_settings/shaders/class1/interface/copyV.glsl"
