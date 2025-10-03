/**
 * @file class1/lighting\lightFuncSpecularV.glsl
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

float calcDirectionalSpecular(vec3 view, vec3 n, vec3 l)
{
    return pow(max(dot(reflect(view, n),l), 0.0),8.0);
}

float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da)
{

    specular.rgb += calcDirectionalSpecular(view,n,l)*lightCol*da;
    return max(dot(n,l),0.0);
}

vec3 calcPointLightSpecular(inout vec4 specular, vec3 view, vec3 v, vec3 n, vec3 l, float r, float pw, vec3 lightCol)
{
    //get light vector
    vec3 lv = l-v;

    //get distance
    float d = length(lv);

    //normalize light vector
    lv *= 1.0/d;

    //distance attenuation
    float da = clamp(1.0/(r * d), 0.0, 1.0);

    //angular attenuation

    da *= calcDirectionalLightSpecular(specular, view, n, lv, lightCol, da);

    return da*lightCol;
}

