/**
 * @file class2\wl\atmosphericsHelpersV.glsl
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

// Output variables

uniform float scene_light_strength;

vec3 atmosFragAmbient(vec3 light, vec3 amblit)
{
    return amblit + light / 2.0;
}

vec3 atmosFragAffectDirectionalLight(float lightIntensity, vec3 sunlit)
{
    return sunlit * lightIntensity;
}

vec3 scaleDownLightFrag(vec3 light)
{
    return (light / scene_light_strength );
}

