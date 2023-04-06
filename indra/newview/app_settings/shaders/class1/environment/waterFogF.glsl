/**
 * @file class1\environment\waterFogF.glsl
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
 


uniform vec4 waterPlane;
uniform vec4 waterFogColor;
uniform float waterFogDensity;
uniform float waterFogKS;

vec3 getPositionEye();

vec3 srgb_to_linear(vec3 col);
vec3 linear_to_srgb(vec3 col);

vec4 applyWaterFogView(vec3 pos, vec4 color)
{
    vec3 view = normalize(pos);
    //normalize view vector
    float es = -(dot(view, waterPlane.xyz));

    //find intersection point with water plane and eye vector
    
    //get eye depth
    float e0 = max(-waterPlane.w, 0.0);
    
    vec3 int_v = waterPlane.w > 0.0 ? view * waterPlane.w/es : vec3(0.0, 0.0, 0.0);
    
    //get object depth
    float depth = length(pos - int_v);
        
    //get "thickness" of water
    float l = max(depth, 0.1);

    float kd = waterFogDensity;
    float ks = waterFogKS;
    vec4 kc = waterFogColor;
    
    float F = 0.98;
    
    float t1 = -kd * pow(F, ks * e0);
    float t2 = kd + ks * es;
    float t3 = pow(F, t2*l) - 1.0;
    
    float L = min(t1/t2*t3, 1.0);
    
    float D = pow(0.98, l*kd);
    
    color.rgb = color.rgb * D + kc.rgb * L;

    return color;
}

vec4 applyWaterFogViewLinearNoClip(vec3 pos, vec4 color, vec3 sunlit)
{
    color.rgb = linear_to_srgb(color.rgb);
    color = applyWaterFogView(pos, color);
    color.rgb = srgb_to_linear(color.rgb);
    return color;
}

vec4 applyWaterFogViewLinear(vec3 pos, vec4 color, vec3 sunlit)
{
    if (dot(pos, waterPlane.xyz) + waterPlane.w > 0.0)
    {
        return color;
    }

    return applyWaterFogViewLinearNoClip(pos, color, sunlit);
}

vec4 applyWaterFogViewLinear(vec3 pos, vec4 color)
{
    return applyWaterFogViewLinear(pos, color, vec3(1));
}

vec4 applyWaterFog(vec4 color)
{
    //normalize view vector
    return applyWaterFogViewLinear(getPositionEye(), color);
}

