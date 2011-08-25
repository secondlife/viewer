/** 
 * @file atmosphericVars.glsl
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
 


varying vec3 vary_PositionEye;

varying vec3 vary_SunlitColor;
varying vec3 vary_AmblitColor;
varying vec3 vary_AdditiveColor;
varying vec3 vary_AtmosAttenuation;

vec3 getPositionEye()
{
	return vary_PositionEye;
}
vec3 getSunlitColor()
{
	return vary_SunlitColor;
}
vec3 getAmblitColor()
{
	return vary_AmblitColor;
}
vec3 getAdditiveColor()
{
	return vary_AdditiveColor;
}
vec3 getAtmosAttenuation()
{
	return vary_AtmosAttenuation;
}


void setPositionEye(vec3 v)
{
	vary_PositionEye = v;
}

void setSunlitColor(vec3 v)
{
	vary_SunlitColor = v;
}

void setAmblitColor(vec3 v)
{
	vary_AmblitColor = v;
}

void setAdditiveColor(vec3 v)
{
	vary_AdditiveColor = v;
}

void setAtmosAttenuation(vec3 v)
{
	vary_AtmosAttenuation = v;
}
