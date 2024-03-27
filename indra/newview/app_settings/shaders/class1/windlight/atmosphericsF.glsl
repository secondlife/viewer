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
vec3 scaleSoftClipFrag(vec3 light);

vec3 srgb_to_linear(vec3 col);
vec3 linear_to_srgb(vec3 col);

uniform float sky_hdr_scale;

vec3 atmosFragLighting(vec3 light, vec3 additive, vec3 atten)
{ 
    light *= atten.r;
    additive = srgb_to_linear(additive*2.0);
    additive *= sky_hdr_scale;
    light += additive;
    return light;
}

vec3 atmosFragLightingLinear(vec3 light, vec3 additive, vec3 atten)
{
    return atmosFragLighting(light, additive, atten);
}

vec3 atmosLighting(vec3 light)
{
    return atmosFragLighting(light, getAdditiveColor(), getAtmosAttenuation());
}
