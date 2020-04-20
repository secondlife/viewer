/** 
 * @file class3/deferred/genSkyShF.glsl
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

VARYING vec2 vary_frag;

uniform vec3 sun_dir;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D single_mie_scattering_texture;
uniform sampler2D irradiance_texture;

vec3 GetSkyLuminance(vec3 camPos, vec3 view_dir, float shadow_length, vec3 dir, out vec3 transmittance);

vec3 calcDirection(vec2 tc)
{
     float phi = tc.y * 2.0 * 3.14159265;
     float cosTheta = sqrt(1.0 - tc.x);
     float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
     return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// reverse mapping above to convert a hemisphere direction into phi/theta values
void getPhiAndThetaFromDirection(vec3 dir, out float phi, out float theta)
{
     float sin_theta;
     float cos_theta;
     cos_theta = dir.z;
     theta     = acos(cos_theta);
     sin_theta = sin(theta);
     phi       = abs(sin_theta) > 0.0001 ? acos(dir.x / sin_theta) : 1.0;
}

// reverse mapping above to convert a hemisphere direction into an SH texture sample pos
vec2 calcShUvFromDirection(vec3 dir)
{
    vec2 uv;
    float phi;
    float theta;
    getPhiAndThetaFromDirection(dir, phi, theta);
    uv.y = phi   / 2.0 * 3.14159265;
    uv.x = theta / 2.0 * 3.14159265;
    return uv;
}

void projectToL1(vec3 n, vec3 c, vec4 basis, out vec4 coeffs[3])
{
    coeffs[0] = vec4(basis.x, n * basis.yzw * c.r);
    coeffs[1] = vec4(basis.x, n * basis.yzw * c.g);
    coeffs[2] = vec4(basis.x, n * basis.yzw * c.b);
}

void main()
{
    float Y00 = sqrt(1.0 / 3.14159265) * 0.5;
    float Y1x = sqrt(3.0 / 3.14159265) * 0.5;
    float Y1y = Y1x;
    float Y1z = Y1x;

    vec4 L1 = vec4(Y00, Y1x, Y1y, Y1z);

    vec3 view_direction = calcDirection(vary_frag);
    vec3 sun_direction  = normalize(sun_dir);
    vec3 cam_pos        = vec3(0, 0, 6360);

    vec3 transmittance;
    vec3 radiance = GetSkyLuminance(cam_pos, view_direction, 0.0f, sun_direction, transmittance);

    vec3 color = vec3(1.0) - exp(-radiance * 0.0001);

    color = pow(color, vec3(1.0/2.2));

    vec4 coeffs[3];
    coeffs[0] = vec4(0);
    coeffs[1] = vec4(0);
    coeffs[2] = vec4(0);

    projectToL1(view_direction, color.rgb, L1, coeffs);

    frag_data[0] = coeffs[0];
    frag_data[1] = coeffs[1];
    frag_data[2] = coeffs[2];
}
