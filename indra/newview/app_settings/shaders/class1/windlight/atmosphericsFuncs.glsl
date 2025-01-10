/**
 * @file class2\windlight\atmosphericsFuncs.glsl
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

uniform vec3  lightnorm;
uniform vec3  sunlight_color;
uniform vec3  moonlight_color;
uniform int   sun_up_factor;
uniform vec3  ambient_color;
uniform vec3  blue_horizon;
uniform vec3  blue_density;
uniform float haze_horizon;
uniform float haze_density;
uniform float cloud_shadow;
uniform float density_multiplier;
uniform float distance_multiplier;
uniform float max_y;
uniform vec3  glow;
uniform float scene_light_strength;
uniform float sun_moon_glow_factor;
uniform float sky_sunlight_scale;
uniform float sky_ambient_scale;
uniform int classic_mode;

float getAmbientClamp() { return 1.0f; }

vec3 srgb_to_linear(vec3 col);

// return colors in sRGB space
void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive,
                         out vec3 atten)
{
    vec3 rel_pos = inPositionEye;

    //(TERRAIN) limit altitude
    if (abs(rel_pos.y) > max_y) rel_pos *= (max_y / rel_pos.y);

    vec3  rel_pos_norm = normalize(rel_pos);
    float rel_pos_len  = length(rel_pos);

    vec3  sunlight     = (sun_up_factor == 1) ? sunlight_color: moonlight_color;

    // sunlight attenuation effect (hue and brightness) due to atmosphere
    // this is used later for sunlight modulation at various altitudes
    vec3 light_atten = (blue_density + vec3(haze_density * 0.25)) * (density_multiplier * max_y);
    // I had thought blue_density and haze_density should have equal weighting,
    // but attenuation due to haze_density tends to seem too strong

    vec3 combined_haze = max(blue_density + vec3(haze_density), vec3(1e-6));
    vec3 blue_weight   = blue_density / combined_haze;
    vec3 haze_weight   = vec3(haze_density) / combined_haze;

    //(TERRAIN) compute sunlight from lightnorm y component. Factor is roughly cosecant(sun elevation) (for short rays like terrain)
    float above_horizon_factor = 1.0 / max(1e-6, lightnorm.y);
    sunlight *= exp(-light_atten * above_horizon_factor);  // for sun [horizon..overhead] this maps to an exp curve [0..1]

    // main atmospheric scattering line integral
    float density_dist = rel_pos_len * density_multiplier;

    // Transparency (-> combined_haze)
    // ATI Bugfix -- can't store combined_haze*density_dist*distance_multiplier in a variable because the ati
    // compiler gets confused.
    combined_haze = exp(-combined_haze * density_dist * distance_multiplier);

    // final atmosphere attenuation factor
    atten = combined_haze.rgb;

    // compute haze glow
    float haze_glow = dot(rel_pos_norm, lightnorm.xyz);

    // dampen sun additive contrib when not facing it...
    // SL-13539: This "if" clause causes an "additive" white artifact at roughly 77 degreees.
    //    if (length(light_dir) > 0.01)
    haze_glow *= max(0.0f, dot(light_dir, rel_pos_norm));

    haze_glow = 1. - haze_glow;
    // haze_glow is 0 at the sun and increases away from sun
    haze_glow = max(haze_glow, .001);  // set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
    haze_glow *= glow.x;
    // higher glow.x gives dimmer glow (because next step is 1 / "angle")
    haze_glow = clamp(pow(haze_glow, glow.z), -100000, 100000);
    // glow.z should be negative, so we're doing a sort of (1 / "angle") function

    // add "minimum anti-solar illumination"
    haze_glow += .25;

    haze_glow *= sun_moon_glow_factor;

    vec3 amb_color = ambient_color;

    // increase ambient when there are more clouds
    vec3 tmpAmbient = amb_color + (vec3(1.) - amb_color) * cloud_shadow * 0.5;

    // Similar/Shared Algorithms:
    //     indra\llinventory\llsettingssky.cpp                                        -- LLSettingsSky::calculateLightSettings()
    //     indra\newview\app_settings\shaders\class1\windlight\atmosphericsFuncs.glsl -- calcAtmosphericVars()
    // haze color
    vec3 cs = sunlight.rgb * (1. - cloud_shadow);
    additive = (blue_horizon.rgb * blue_weight.rgb) * (cs + tmpAmbient.rgb) + (haze_horizon * haze_weight.rgb) * (cs * haze_glow + tmpAmbient.rgb);

    // brightness of surface both sunlight and ambient

    sunlit = sunlight.rgb;
    amblit = pow(tmpAmbient.rgb, vec3(0.9)) * 0.57;

    additive *= vec3(1.0 - combined_haze);

    // sanity clamp haze contribution
    additive = min(additive, vec3(10));
}

vec3 srgb_to_linear(vec3 col);

// provide a touch of lighting in the opposite direction of the sun light
    // so areas in shadow don't lose all detail
float ambientLighting(vec3 norm, vec3 light_dir)
{
    float ambient = min(abs(dot(norm.xyz, light_dir.xyz)), 1.0);
    ambient *= 0.5;
    ambient *= ambient;
    ambient = (1.0 - ambient);
    return ambient;
}

// return lit amblit in linear space, leave sunlit and additive in sRGB space
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 additive,
                         out vec3 atten)
{
    calcAtmosphericVars(inPositionEye, light_dir, 1.0, sunlit, amblit, additive, atten);

    amblit *= ambientLighting(norm, light_dir);

    if (classic_mode < 1)
    {
        amblit = srgb_to_linear(amblit);
        sunlit = srgb_to_linear(sunlit);
    }

    // multiply to get similar colors as when the "scaleSoftClip" implementation was doubling color values
    // (allows for mixing of light sources other than sunlight e.g. reflection probes)
    sunlit *= sky_sunlight_scale;
    amblit *= sky_ambient_scale;
}
