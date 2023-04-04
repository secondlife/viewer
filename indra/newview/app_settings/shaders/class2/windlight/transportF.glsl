/** 
 * @file class2\wl\transportF.glsl
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
 
//////////////////////////////////////////////////////////
// The fragment shader for the terrain atmospherics
//////////////////////////////////////////////////////////

vec3 getAdditiveColor();
vec3 getAtmosAttenuation();

vec3 atmosFragLighting(vec3 light, vec3 additive, vec3 atten);

// the below implementations are deprecated but remain here as adapters for shaders that haven't been refactored yet

vec3 atmosTransportFrag(vec3 light, vec3 additive, vec3 atten)
{
    return atmosFragLighting(light, additive, atten);
}

vec3 atmosTransport(vec3 light)
{
     return atmosTransportFrag(light, getAdditiveColor(), getAtmosAttenuation());
}

vec3 fullbrightAtmosTransportFrag(vec3 light, vec3 additive, vec3 atten)
{
    return atmosTransportFrag(light, additive, atten);
}

vec3 fullbrightAtmosTransport(vec3 light)
{
    return atmosTransport(light);
}

vec3 fullbrightShinyAtmosTransport(vec3 light)
{
    return atmosTransport(light);
}
