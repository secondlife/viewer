/** 
 * @file gammaF.glsl
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
 


uniform vec4 gamma;
uniform int no_atmo;

vec3 getAtmosAttenuation();

/// Soft clips the light with a gamma correction
vec3 scaleSoftClip(vec3 light) {
    if (no_atmo == 1)
    {
        return light;
    }
	//soft clip effect:
	light = 1. - clamp(light, vec3(0.), vec3(1.));
	light = 1. - pow(light, gamma.xxx);

	return light;
}

vec3 fullbrightScaleSoftClipFrag(vec3 light, vec3 atten)
{
	return (no_atmo == 1) ? light : mix(scaleSoftClip(light.rgb), light.rgb, atten);
}

vec3 fullbrightScaleSoftClip(vec3 light) {
	return (no_atmo == 1) ? light : fullbrightScaleSoftClipFrag(light.rgb, getAtmosAttenuation());
}

