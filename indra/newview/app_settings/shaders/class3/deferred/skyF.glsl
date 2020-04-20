/** 
 * @file class3/deferred/skyF.glsl
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

uniform vec3 camPosLocal;
uniform vec3 sun_dir;
uniform float sun_size;
uniform float far_z;
uniform mat4 inv_proj;
uniform mat4 inv_modelview;

uniform sampler2D transmittance_texture;
uniform sampler3D scattering_texture;
uniform sampler3D single_mie_scattering_texture;
uniform sampler2D irradiance_texture;
uniform sampler2D rainbow_map;
uniform sampler2D halo_map;

uniform float moisture_level;
uniform float droplet_radius;
uniform float ice_level;

vec3 GetSolarLuminance();
vec3 GetSkyLuminance(vec3 camPos, vec3 view_dir, float shadow_length, vec3 dir, out vec3 transmittance);
vec3 GetSkyLuminanceToPoint(vec3 camPos, vec3 pos, float shadow_length, vec3 dir, out vec3 transmittance);

vec3 ColorFromRadiance(vec3 radiance);
vec3 rainbow(float d)
{
   float rad = (droplet_radius - 5.0f) / 1024.0f;
   return pow(texture2D(rainbow_map, vec2(rad, d)).rgb, vec3(1.8)) * moisture_level;
}

vec3 halo22(float d)
{
   float v = sqrt(max(0, 1 - (d*d)));
   return texture2D(halo_map, vec2(0, v)).rgb * ice_level;
}

void main()
{
    vec3 pos      = vec3((vary_frag * 2.0) - vec2(1.0, 1.0f), 1.0);
    vec4 view_pos = (inv_proj * vec4(pos, 1.0f));

    view_pos /= view_pos.w;

    vec3 view_ray = (inv_modelview * vec4(view_pos.xyz, 0.0f)).xyz + camPosLocal;

    vec3 view_direction = normalize(view_ray);
    vec3 sun_direction  = normalize(sun_dir);
    vec3 earth_center   = vec3(0, 0, -6360.0f);
    vec3 camPos = (camPosLocal / 1000.0f) - earth_center;

    vec3 transmittance;
    vec3 radiance_sun  = GetSkyLuminance(camPos, view_direction, 0.0f, sun_direction, transmittance);
    vec3 solar_luminance = GetSolarLuminance();

    // If the view ray intersects the Sun, add the Sun radiance.
    float s = dot(view_direction, sun_direction);

    // cheesy solar disc...
    if (s >= (sun_size * 0.999))
    {
        radiance_sun += pow(smoothstep(0.0, 1.3, (s - (sun_size * 0.9))), 2.0) * solar_luminance * transmittance;
    }
    s = smoothstep(0.9, 1.0, s) * 16.0f;

    vec3 color = ColorFromRadiance(radiance_sun);

    float optic_d = dot(view_direction, sun_direction);
    vec3 halo_22 = halo22(optic_d);

    color.rgb += rainbow(optic_d) * optic_d;
    color.rgb += halo_22;

    color = pow(color, vec3(1.0/2.2));

    frag_data[0] = vec4(color, 1.0 + s);
    frag_data[1] = vec4(0.0);
    frag_data[2] = vec4(0.0, 1.0, 0.0, 1.0);
}

