/** 
 * @file class2\wl\atmosphericsF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
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

vec3 getAdditiveColor();
vec3 getAtmosAttenuation();

uniform vec4 gamma;
uniform vec4 lightnorm;
uniform vec4 sunlight_color;
uniform vec4 moonlight_color;
uniform int sun_up_factor;
uniform vec4 ambient;
uniform vec4 blue_horizon;
uniform vec4 blue_density;
uniform float haze_horizon;
uniform float haze_density;
uniform float cloud_shadow;
uniform float density_multiplier;
uniform float distance_multiplier;
uniform float max_y;
uniform vec4 glow;
uniform float scene_light_strength;
uniform mat3 ssao_effect_mat;
uniform int no_atmo;
uniform float sun_moon_glow_factor;

vec3 scaleSoftClipFrag(vec3 light);

vec3 atmosFragLighting(vec3 light, vec3 additive, vec3 atten)
{
    if (no_atmo == 1)
    {
        return light;
    }
    light *= atten.r;
    light += additive;
    return light * 2.0;
}

vec3 atmosLighting(vec3 light)
{
    return atmosFragLighting(light, getAdditiveColor(), getAtmosAttenuation());
}
