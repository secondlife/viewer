/** 
 * @file class3/deferred/cloudsF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

VARYING vec4 vary_CloudColorSun;
VARYING vec4 vary_CloudColorAmbient;
VARYING float vary_CloudDensity;
VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;
VARYING vec2 vary_texcoord3;
VARYING vec3 vary_pos;

uniform sampler2D cloud_noise_texture;
uniform sampler2D cloud_noise_texture_next;
uniform float blend_factor;
uniform vec4 cloud_pos_density1;
uniform vec4 cloud_pos_density2;
uniform vec4 cloud_color;
uniform float cloud_shadow;
uniform float cloud_scale;
uniform float cloud_variance;
uniform vec3 camPosLocal;
uniform vec3 sun_dir;
uniform float sun_size;
uniform float far_z;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D single_mie_scattering_texture;
uniform sampler2D irradiance_texture;
uniform sampler2D sh_input_r;
uniform sampler2D sh_input_g;
uniform sampler2D sh_input_b;

vec3 GetSkyLuminance(vec3 camPos, vec3 view_dir, float shadow_length, vec3 dir, out vec3 transmittance);

/// Soft clips the light with a gamma correction
vec3 scaleSoftClip(vec3 light);

vec4 cloudNoise(vec2 uv)
{
   vec4 a = texture2D(cloud_noise_texture, uv);
   vec4 b = texture2D(cloud_noise_texture_next, uv);
   vec4 cloud_noise_sample = mix(a, b, blend_factor);
   return cloud_noise_sample;
}

void main()
{
    // Set variables
    vec2 uv1 = vary_texcoord0.xy;
    vec2 uv2 = vary_texcoord1.xy;
    vec2 uv3 = vary_texcoord2.xy;
    float cloudDensity = 2.0 * (cloud_shadow - 0.25);

    if (cloud_scale < 0.001)
    {
        discard;
    }

    vec2 uv4 = vary_texcoord3.xy;

    vec2 disturbance = vec2(cloudNoise(uv1 / 16.0f).x, cloudNoise((uv3 + uv1) / 16.0f).x) * cloud_variance * (1.0f - cloud_scale * 0.25f);

    // Offset texture coords
    uv1 += cloud_pos_density1.xy + (disturbance * 0.2); //large texture, visible density
    uv2 += cloud_pos_density1.xy;   //large texture, self shadow
    uv3 += cloud_pos_density2.xy; //small texture, visible density
    uv4 += cloud_pos_density2.xy;   //small texture, self shadow

    float density_variance = min(1.0, (disturbance.x* 2.0 + disturbance.y* 2.0) * 4.0);

    cloudDensity *= 1.0 - (density_variance * density_variance);

    // Compute alpha1, the main cloud opacity
    float alpha1 = (cloudNoise(uv1).x - 0.5) + (cloudNoise(uv3).x - 0.5) * cloud_pos_density2.z;
    alpha1 = min(max(alpha1 + cloudDensity, 0.) * 10 * cloud_pos_density1.z, 1.);

    // And smooth
    alpha1 = 1. - alpha1 * alpha1;
    alpha1 = 1. - alpha1 * alpha1;  

    if (alpha1 < 0.001f)
    {
        discard;
    }

    // Compute alpha2, for self shadowing effect
    // (1 - alpha2) will later be used as percentage of incoming sunlight
    float alpha2 = (cloudNoise(uv2).x - 0.5);
    alpha2 = min(max(alpha2 + cloudDensity, 0.) * 2.5 * cloud_pos_density1.z, 1.);

    // And smooth
    alpha2 = 1. - alpha2;
    alpha2 = 1. - alpha2 * alpha2;  

    vec3 view_ray = vary_pos.xyz + camPosLocal;

    vec3 view_direction = normalize(view_ray);
    vec3 sun_direction  = normalize(sun_dir);
    vec3 earth_center   = vec3(0, 0, -6360.0f);
    vec3 camPos = (camPosLocal / 1000.0f) - earth_center;

    vec3 transmittance;
    vec3 radiance_sun  = GetSkyLuminance(camPos, view_direction, 1.0 - alpha1, sun_direction, transmittance);

    vec3 sun_color = vec3(1.0) - exp(-radiance_sun * 0.0001);

    // Combine
    vec4 color;

    vec4 l1tap = vec4(1.0/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265), sqrt(3)/sqrt(4*3.14159265));

    vec4 l1r = texture2D(sh_input_r, vec2(0,0));
    vec4 l1g = texture2D(sh_input_g, vec2(0,0));
    vec4 l1b = texture2D(sh_input_b, vec2(0,0));

    vec3 sun_indir = vec3(-view_direction.xy, view_direction.z);
    vec3 amb = vec3(dot(l1r, l1tap * vec4(1, sun_indir)),
                    dot(l1g, l1tap * vec4(1, sun_indir)),
                    dot(l1b, l1tap * vec4(1, sun_indir)));


    amb = max(vec3(0), amb);

    color.rgb = sun_color * cloud_color.rgb * (1. - alpha2);
    color.rgb = pow(color.rgb, vec3(1.0 / 2.2));
    color.rgb += amb;

    frag_data[0] = vec4(color.rgb, alpha1);
    frag_data[1] = vec4(0);
    frag_data[2] = vec4(0,1,0,1);
}

