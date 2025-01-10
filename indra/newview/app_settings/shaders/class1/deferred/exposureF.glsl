/**
 * @file exposureF.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D emissiveRect;
#ifdef USE_LAST_EXPOSURE
uniform sampler2D exposureMap;
#endif

uniform float dt;
uniform vec2 noiseVec;

uniform vec4 dynamic_exposure_params;
uniform vec4 dynamic_exposure_params2;

float lum(vec3 col)
{
    vec3 l = vec3(0.2126, 0.7152, 0.0722);
    return dot(l, col);
}

void main()
{
    vec2 tc = vec2(0.5,0.5);

    float L = textureLod(emissiveRect, tc, 8).r;
    float max_L = dynamic_exposure_params.x;
    L = clamp(L, 0.0, max_L);
    L /= max_L;
    L = pow(L, 2.0);
    float s = mix(dynamic_exposure_params.z, dynamic_exposure_params.y, L);
#ifdef USE_LAST_EXPOSURE
    float prev = texture(exposureMap, vec2(0.5,0.5)).r;

    float speed = -log(dynamic_exposure_params.w) / dynamic_exposure_params2.w;
    s = mix(prev, s, 1 - exp(-speed * dt));
#endif

    frag_color = max(vec4(s, s, s, dt), vec4(0.0));
}

