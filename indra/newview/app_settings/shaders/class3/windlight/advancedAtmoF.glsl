/** 
 * @file class3\wl\advancedAtmoF.glsl
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
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

in vec3 view_dir;

uniform vec3 cameraPosLocal;
uniform vec3 sun_dir;
uniform float sun_size;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D mie_scattering_texture;
uniform sampler2D irradiance_texture;

vec3 GetSolarLuminance();
vec3 GetSkyLuminance(vec3 camPos, vec3 view_dir, float shadow_length, vec3 sun_dir, out vec3 transmittance);
vec3 GetSkyLuminanceToPoint(vec3 camPos, vec3 pos, float shadow_length, vec3 sun_dir, out vec3 transmittance);
vec3 GetSunAndSkyIlluminance(vec3 pos, vec3 norm, vec3 sun_dir, out vec3 sky_irradiance);

void main()
{
    vec3 view_direction = normalize(view_dir);

    vec3 camPos = cameraPosLocal;
    vec3 transmittance;
    vec3 sky_illum;
    vec3 radiance = GetSkyLuminance(camPos, view_direction, 0.0f, sun_dir, transmittance);
    vec3 radiance2 = GetSunAndSkyIlluminance(camPos, view_direction, sun_dir, sky_illum);

    //radiance *= transmittance;

    // If the view ray intersects the Sun, add the Sun radiance.
    if (dot(view_direction, sun_dir) >= sun_size)
    {
        radiance = radiance + transmittance * GetSolarLuminance();
    }

    //vec3 color = vec3(1.0) - exp(-radiance);
    //color = pow(color, vec3(1.0 / 2.2));

    frag_color.rgb = radiance;
 
    frag_color.a = 1.0;
}

