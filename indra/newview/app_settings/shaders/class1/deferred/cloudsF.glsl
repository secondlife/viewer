/**
 * @file class1\deferred\cloudsF.glsl
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
/*[EXTRA_CODE_HERE]*/

out vec4 frag_data[4];

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

// In
in vec3 pos;

in vec2 vary_texcoord0;
in vec2 vary_texcoord1;
in vec2 vary_texcoord2;
in vec2 vary_texcoord3;

uniform sampler2D cloud_noise_texture;
uniform sampler2D cloud_noise_texture_next;
uniform float blend_factor;
uniform vec3 cloud_pos_density1;
uniform vec3 cloud_pos_density2;
uniform float cloud_scale;
uniform float cloud_variance;

uniform vec3 camPosLocal;

uniform vec3 lightnorm;
uniform vec3 sunlight_color;
uniform vec3 moonlight_color;
uniform int sun_up_factor;
uniform vec3 ambient_color;
uniform vec3 blue_horizon;
uniform vec3 blue_density;
uniform float haze_horizon;
uniform float haze_density;

uniform float cloud_shadow;
uniform float density_multiplier;
uniform float max_y;

uniform vec3 glow;
uniform float sun_moon_glow_factor;

uniform vec3 cloud_color;

vec4 cloudNoise(vec2 uv)
{
   vec4 a = texture(cloud_noise_texture, uv);
   vec4 b = texture(cloud_noise_texture_next, uv);
   vec4 cloud_noise_sample = mix(a, b, blend_factor);
   return cloud_noise_sample;
}

void main()
{
    if (cloud_scale < 0.001)
    {
        discard;
    }

    // Set variables
    vec2 uv1 = vary_texcoord0.xy;
    vec2 uv2 = vary_texcoord1.xy;
    vec2 uv3 = vary_texcoord2.xy;
    vec2 uv4 = vary_texcoord3.xy;

    // Get relative position
    vec3 rel_pos = pos.xyz - camPosLocal.xyz + vec3(0, 50, 0);

    float altitude_blend_factor = clamp((rel_pos.y + 512.0) / max_y, 0.0, 1.0);

    // Set altitude
    if (rel_pos.y > 0)
    {
        rel_pos *= (max_y / rel_pos.y);
    }
    if (rel_pos.y < 0)
    {
        altitude_blend_factor = 0; // SL-11589 Fix clouds drooping below horizon
        rel_pos *= (-32000. / rel_pos.y);
    }

    // Can normalize then
    vec3  rel_pos_norm = normalize(rel_pos);
    float rel_pos_len  = length(rel_pos);

    // Initialize temp variables
    vec3 sunlight = sunlight_color;
    vec3 light_atten;

    // Sunlight attenuation effect (hue and brightness) due to atmosphere
    // this is used later for sunlight modulation at various altitudes
    light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);

    // Calculate relative weights
    vec3 combined_haze = abs(blue_density) + vec3(abs(haze_density));
    vec3 blue_weight   = blue_density / combined_haze;
    vec3 haze_weight   = haze_density / combined_haze;

    // Compute sunlight from rel_pos & lightnorm (for long rays like sky)
    float off_axis = 1.0 / max(1e-6, max(0., rel_pos_norm.y) + lightnorm.y);
    sunlight *= exp(-light_atten * off_axis);

    // Distance
    float density_dist = rel_pos_len * density_multiplier;

    // Transparency (-> combined_haze)
    // ATI Bugfix -- can't store combined_haze*density_dist in a variable because the ati
    // compiler gets confused.
    combined_haze = exp(-combined_haze * density_dist);

    // Compute haze glow
    float haze_glow = 1.0 - dot(rel_pos_norm, lightnorm.xyz);
    // haze_glow is 0 at the sun and increases away from sun
    haze_glow = max(haze_glow, .001);
    // Set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
    haze_glow *= glow.x;
    // Higher glow.x gives dimmer glow (because next step is 1 / "angle")
    haze_glow = pow(haze_glow, glow.z);
    // glow.z should be negative, so we're doing a sort of (1 / "angle") function

    haze_glow *= sun_moon_glow_factor;

    // Add "minimum anti-solar illumination"
    // For sun, add to glow.  For moon, remove glow entirely. SL-13768
    haze_glow = (sun_moon_glow_factor < 1.0) ? 0.0 : (haze_glow + 0.25);

    // Increase ambient when there are more clouds
    vec3 tmpAmbient = ambient_color;
    tmpAmbient += (1. - tmpAmbient) * cloud_shadow * 0.5;

    // Dim sunlight by cloud shadow percentage
    sunlight *= (1. - cloud_shadow);

    // Haze color below cloud
    vec3 additiveColorBelowCloud =
        (blue_horizon * blue_weight * (sunlight + tmpAmbient) + (haze_horizon * haze_weight) * (sunlight * haze_glow + tmpAmbient));

    // CLOUDS
    sunlight = sunlight_color;
    off_axis = 1.0 / max(1e-6, lightnorm.y * 2.);
    sunlight *= exp(-light_atten * off_axis);

    // Cloud color out
    vec3 cloudColorSun = (sunlight * haze_glow) * cloud_color;
    vec3 cloudColorAmbient = tmpAmbient * cloud_color;

    // Attenuate cloud color by atmosphere
    combined_haze = sqrt(combined_haze);  // less atmos opacity (more transparency) below clouds
    cloudColorSun *= combined_haze;
    cloudColorAmbient *= combined_haze;
    vec3 oHazeColorBelowCloud = additiveColorBelowCloud * (1. - combined_haze);

    // Make a nice cloud density based on the cloud_shadow value that was passed in.
    float cloudDensity = 2. * (cloud_shadow - 0.25);

    // Combine these to minimize register use
    cloudColorAmbient += oHazeColorBelowCloud;

    // Cloud Fragment
    vec2 disturbance  = vec2(cloudNoise(uv1 / 8.0f).x, cloudNoise((uv3 + uv1) / 16.0f).x) * cloud_variance * (1.0f - cloud_scale * 0.25f);
    vec2 disturbance2 = vec2(cloudNoise((uv1 + uv3) / 4.0f).x, cloudNoise((uv4 + uv2) / 8.0f).x) * cloud_variance * (1.0f - cloud_scale * 0.25f);

    // Offset texture coords
    uv1 += cloud_pos_density1.xy + (disturbance * 0.2);    //large texture, visible density
    uv2 += cloud_pos_density1.xy;   //large texture, self shadow
    uv3 += cloud_pos_density2.xy;   //small texture, visible density
    uv4 += cloud_pos_density2.xy;   //small texture, self shadow

    float density_variance = min(1.0, (disturbance.x* 2.0 + disturbance.y* 2.0 + disturbance2.x + disturbance2.y) * 4.0);

    cloudDensity *= 1.0 - (density_variance * density_variance);

    // Compute alpha1, the main cloud opacity

    float alpha1 = (cloudNoise(uv1).x - 0.5) + (cloudNoise(uv3).x - 0.5) * cloud_pos_density2.z;
    alpha1 = min(max(alpha1 + cloudDensity, 0.) * 10 * cloud_pos_density1.z, 1.);

    // And smooth
    alpha1 = 1. - alpha1 * alpha1;
    alpha1 = 1. - alpha1 * alpha1;

    alpha1 *= altitude_blend_factor;
    alpha1 = clamp(alpha1, 0.0, 1.0);

    // Compute alpha2, for self shadowing effect
    // (1 - alpha2) will later be used as percentage of incoming sunlight
    float alpha2 = (cloudNoise(uv2).x - 0.5);
    alpha2 = min(max(alpha2 + cloudDensity, 0.) * 2.5 * cloud_pos_density1.z, 1.);

    // And smooth
    alpha2 = 1. - alpha2;
    alpha2 = 1. - alpha2 * alpha2;

    // Combine
    vec3 color;
    color = (cloudColorSun*(1.-alpha2) + cloudColorAmbient);
    color.rgb = clamp(color.rgb, vec3(0), vec3(1));
    color.rgb *= 2.0;

    /// Gamma correct for WL (soft clip effect).

    frag_data[1] = vec4(0.0,0.0,0.0,0.0);
    frag_data[2] = vec4(0,0,0,GBUFFER_FLAG_SKIP_ATMOS);

#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0);
    frag_data[3] = vec4(color.rgb, alpha1);
#else
    frag_data[0] = vec4(color.rgb, alpha1);
#endif
}

