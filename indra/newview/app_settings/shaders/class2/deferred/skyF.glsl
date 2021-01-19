/**
 * @file class2/deferred/skyF.glsl
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

uniform mat4 modelview_projection_matrix;

// SKY ////////////////////////////////////////////////////////////////////////
// The vertex shader for creating the atmospheric sky
///////////////////////////////////////////////////////////////////////////////

// Inputs
uniform vec3 camPosLocal;

uniform vec4  lightnorm;
uniform vec4  sunlight_color;
uniform vec4  moonlight_color;
uniform int   sun_up_factor;
uniform vec4  ambient_color;
uniform vec4  blue_horizon;
uniform vec4  blue_density;
uniform float haze_horizon;
uniform float haze_density;

uniform float cloud_shadow;
uniform float density_multiplier;
uniform float distance_multiplier;
uniform float max_y;

uniform vec4  glow;
uniform float sun_moon_glow_factor;

uniform vec4 cloud_color;

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

VARYING vec3 pos;

/////////////////////////////////////////////////////////////////////////
// The fragment shader for the sky
/////////////////////////////////////////////////////////////////////////

uniform sampler2D rainbow_map;
uniform sampler2D halo_map;

uniform float moisture_level;
uniform float droplet_radius;
uniform float ice_level;

vec3 rainbow(float d)
{
    // d is the dot product of view and sun directions, so ranging -1.0..1.0
    // 'interesting' values of d are the range -0.75..-0.825, when view is nearly opposite of sun vec
    // Rainbox texture mode is GL_REPEAT, so tc of -.75 is equiv to 0.25, -0.825 equiv to 0.175.

    // SL-13629 Rainbow texture has colors within the correct .175...250 range, but order is inverted.
    // Rather than replace the texture, we mirror and translate the y tc to keep the colors within the
    // interesting range, but in reversed order:  i.e. d = (1 - d) - 1.575
    d = clamp(-0.575 - d, 0.0, 1.0);

    // With the colors in the lower 1/4 of the texture, inverting the coords leaves most of it inaccessible.
    // So, we can stretch the texcoord above the colors (ie > 0.25) to fill the entire remaining coordinate
    // space. This improves gradation, reduces banding within the rainbow interior. (1-0.25) / (0.425/0.25) = 4.2857
    float interior_coord = max(0.0, d - 0.25) * 4.2857;
    d = clamp(d, 0.0, 0.25) + interior_coord;

    float rad = (droplet_radius - 5.0f) / 1024.0f;
    return pow(texture2D(rainbow_map, vec2(rad, d)).rgb, vec3(1.8)) * moisture_level;
}

vec3 halo22(float d)
{
    d       = clamp(d, 0.1, 1.0);
    float v = sqrt(clamp(1 - (d * d), 0, 1));
    return texture2D(halo_map, vec2(0, v)).rgb * ice_level;
}

/// Soft clips the light with a gamma correction
vec3 scaleSoftClip(vec3 light);

void main()
{
    // World / view / projection
    // Get relative position (offset why?)
    vec3 rel_pos = pos.xyz - camPosLocal.xyz + vec3(0, 50, 0);

    // Adj position vector to clamp altitude
    if (rel_pos.y > 0.)
    {
        rel_pos *= (max_y / rel_pos.y);
    }
    if (rel_pos.y < 0.)
    {
        rel_pos *= (-32000. / rel_pos.y);
    }

    // Normalized
    vec3  rel_pos_norm = normalize(rel_pos);
    float rel_pos_len  = length(rel_pos);

    // Initialize temp variables
    vec4 sunlight = (sun_up_factor == 1) ? sunlight_color : moonlight_color;

    // Sunlight attenuation effect (hue and brightness) due to atmosphere
    // this is used later for sunlight modulation at various altitudes
    vec4 light_atten = (blue_density + vec4(haze_density * 0.25)) * (density_multiplier * max_y);

    // Calculate relative weights
    vec4 combined_haze = abs(blue_density) + vec4(abs(haze_density));
    vec4 blue_weight   = blue_density / combined_haze;
    vec4 haze_weight   = haze_density / combined_haze;

    // Compute sunlight from rel_pos & lightnorm (for long rays like sky)
    float off_axis = 1.0 / max(1e-6, max(0, rel_pos_norm.y) + lightnorm.y);
    sunlight *= exp(-light_atten * off_axis);

    // Distance
    float density_dist = rel_pos_len * density_multiplier;

    // Transparency (-> combined_haze)
    // ATI Bugfix -- can't store combined_haze*density_dist in a variable because the ati
    // compiler gets confused.
    combined_haze = exp(-combined_haze * density_dist);

    // Compute haze glow
    float haze_glow = dot(rel_pos_norm, lightnorm.xyz);
    haze_glow       = 1. - haze_glow;
    // haze_glow is 0 at the sun and increases away from sun
    haze_glow = max(haze_glow, .001);
    // Set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
    haze_glow *= glow.x;
    // Higher glow.x gives dimmer glow (because next step is 1 / "angle")
    haze_glow = pow(haze_glow, glow.z);
    // glow.z should be negative, so we're doing a sort of (1 / "angle") function

    // Add "minimum anti-solar illumination"
    // For sun, add to glow.  For moon, remove glow entirely. SL-13768
    haze_glow = (sun_moon_glow_factor < 1.0) ? 0.0 : (sun_moon_glow_factor * (haze_glow + 0.25));

    // Haze color above cloud
    vec4 color = blue_horizon * blue_weight * (sunlight + ambient_color) 
               + haze_horizon * haze_weight * (sunlight * haze_glow + ambient_color);

    // Final atmosphere additive
    color *= (1. - combined_haze);

    // Increase ambient when there are more clouds
    // TODO 9/20: DJH what does this do?  max(0,(1-ambient)) will change the color
    vec4 ambient = ambient_color + max(vec4(0), (1. - ambient_color)) * cloud_shadow * 0.5;

    // Dim sunlight by cloud shadow percentage
    sunlight *= max(0.0, (1. - cloud_shadow));

    // Haze color below cloud
    vec4 add_below_cloud = blue_horizon * blue_weight * (sunlight + ambient) 
                         + haze_horizon * haze_weight * (sunlight * haze_glow + ambient);

    // Attenuate cloud color by atmosphere
    combined_haze = sqrt(combined_haze);  // less atmos opacity (more transparency) below clouds

    // At horizon, blend high altitude sky color towards the darker color below the clouds
    color += (add_below_cloud - color) * (1. - sqrt(combined_haze));

    float optic_d = dot(rel_pos_norm, lightnorm.xyz);
    vec3  halo_22 = halo22(optic_d);
    color.rgb += rainbow(optic_d);
    color.rgb += halo_22;
    color.rgb *= 2.;
    color.rgb = scaleSoftClip(color.rgb);

    // Gamma correct for WL (soft clip effect).
    frag_data[0] = vec4(color.rgb, 1.0);
    frag_data[1] = vec4(0.0, 0.0, 0.0, 0.0);
    frag_data[2] = vec4(0.0, 0.0, 0.0, 1.0);  // 1.0 in norm.w masks off fog
}
