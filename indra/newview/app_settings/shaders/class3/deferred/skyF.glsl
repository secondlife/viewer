/** 
 * @file advancedAtmoF.glsl
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

in vec3 view_pos;
in vec3 view_dir;

uniform vec3 cameraPosLocal;
uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform float sun_size;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D single_mie_scattering_texture;
uniform sampler2D irradiance_texture;

vec3 GetSolarLuminance();
vec3 GetSkyLuminance(vec3 camPos, vec3 view_dir, float shadow_length, vec3 dir, out vec3 transmittance);
vec3 GetSkyLuminanceToPoint(vec3 camPos, vec3 pos, float shadow_length, vec3 dir, out vec3 transmittance);
vec3 GetSunAndSkyIlluminance(vec3 pos, vec3 norm, vec3 dir, out vec3 sky_irradiance);

void main()
{
    vec3 view_direction = normalize(view_dir);

    vec3 sun_direction = sun_dir;

    vec3 camPos = cameraPosLocal + vec3(0, 0, 6360.0f);
    vec3 transmittance;
    vec3 sky_illum;

    vec3 radiance_sun = GetSkyLuminance(camPos, view_direction, 0.0f, sun_direction, transmittance);
    vec3 radiance2_sun = GetSunAndSkyIlluminance(camPos, view_direction, sun_direction, sky_illum);

    radiance *= transmittance;

    vec3 solar_luminance = transmittance * GetSolarLuminance();

    // If the view ray intersects the Sun, add the Sun radiance.
    if (dot(view_direction, sun_direction) >= sun_size)
    {
        radiance = radiance + solar_luminance;
    }

    vec3 color = radiance;
    
    color = vec3(1.0) - exp(-color * 0.0001);

    //float d = dot(view_direction, sun_direction);
    //frag_color.rgb = vec3(d, d >= sun_size ? 1.0f : 0.0f, 0.0f);

    frag_color.rgb = color;
    //frag_color.rgb = vec3(dot(view_direction, sun_direction) > 0.95f ? 1.0 : 0.0, 0,0);
    frag_color.rgb = normalize(view_pos);

    frag_color.a = 1.0;
}

