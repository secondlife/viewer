/**
 * @file class2/deferred/reflectionProbeF.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

// Implementation for when reflection probes are disabled

uniform float reflection_probe_ambiance;

uniform samplerCube environmentMap;

uniform mat3 env_mat;

vec3 srgb_to_linear(vec3 c);

void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness)
{
    ambenv = vec3(reflection_probe_ambiance * 0.25);
    
    vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));
    vec3 env_vec = env_mat * refnormpersp;
    glossenv = srgb_to_linear(textureCube(environmentMap, env_vec).rgb);
}

void sampleReflectionProbesWater(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness)
{
    sampleReflectionProbes(ambenv, glossenv, tc, pos, norm, glossiness);
}

vec4 sampleReflectionProbesDebug(vec3 pos)
{
    // show nothing in debug display
    return vec4(0, 0, 0, 0);
}

void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, float envIntensity)
{
    ambenv = vec3(reflection_probe_ambiance * 0.25);
    
    vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));
    vec3 env_vec = env_mat * refnormpersp;

    legacyenv = srgb_to_linear(textureCube(environmentMap, env_vec).rgb);

    glossenv = legacyenv;
}

void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm)
{
    
}

void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity)
{
    color = mix(color.rgb, legacyenv*1.5, envIntensity);
}

