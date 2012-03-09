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
 

VARYING vec3 vary_AdditiveColor;
VARYING vec3 vary_AtmosAttenuation;

vec3 additive_color;
vec3 atmos_attenuation;
vec3 sunlit_color;
vec3 amblit_color;
vec3 position_eye;

vec3 getSunlitColor()
{
	return sunlit_color;
}
vec3 getAmblitColor()
{
	return amblit_color;
}

vec3 getAdditiveColor()
{
	return additive_color;
}
vec3 getAtmosAttenuation()
{
	return atmos_attenuation;
}

vec3 getPositionEye()
{
	return position_eye;
}

void setPositionEye(vec3 v)
{
	position_eye = v;
}

void setSunlitColor(vec3 v)
{
	sunlit_color  = v;
}

void setAmblitColor(vec3 v)
{
	amblit_color = v;
}

void setAdditiveColor(vec3 v)
{
	additive_color = v;
	vary_AdditiveColor = v;
}

void setAtmosAttenuation(vec3 v)
{
	atmos_attenuation = v;
	vary_AtmosAttenuation = v;
}
