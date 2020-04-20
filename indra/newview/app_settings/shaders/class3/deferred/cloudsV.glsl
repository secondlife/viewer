/** 
 * @file WLCloudsV.glsl
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

uniform mat4 modelview_projection_matrix;
uniform mat4 modelview_matrix;

ATTRIBUTE vec3 position;
ATTRIBUTE vec2 texcoord0;

//////////////////////////////////////////////////////////////////////////
// The vertex shader for creating the atmospheric sky
///////////////////////////////////////////////////////////////////////////////

// Output parameters
VARYING vec2 vary_texcoord0;
VARYING vec2 vary_texcoord1;
VARYING vec2 vary_texcoord2;
VARYING vec2 vary_texcoord3;
VARYING vec3 vary_pos;

// Inputs
uniform float cloud_scale;
uniform vec4  lightnorm;
uniform vec3  camPosLocal;

void main()
{
    vary_pos = position;

    // World / view / projection
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0);

    // Texture coords
    vary_texcoord0 = texcoord0;
    vary_texcoord0.xy -= 0.5;
    vary_texcoord0.xy /= max(0.001, cloud_scale);
    vary_texcoord0.xy += 0.5;

    vary_texcoord1 = vary_texcoord0;
    vary_texcoord1.x += lightnorm.x * 0.0125;
    vary_texcoord1.y += lightnorm.z * 0.0125;

    vary_texcoord2 = vary_texcoord0 * 16.;
    vary_texcoord3 = vary_texcoord1 * 16.;

    // END CLOUDS
}

