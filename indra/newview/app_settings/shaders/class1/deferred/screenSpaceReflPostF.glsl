/**
 * @file class3/deferred/screenSpaceReflPostF.glsl
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

#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform vec2 screen_res;
uniform mat4 projection_matrix;
uniform mat4 inv_proj;
uniform float zNear;
uniform float zFar;

VARYING vec2 vary_fragcoord;

uniform sampler2D depthMap;
uniform sampler2D normalMap;
uniform sampler2D sceneMap;
uniform sampler2D diffuseRect;

vec3 getNorm(vec2 screenpos);
float getDepth(vec2 pos_screen);
float linearDepth(float d, float znear, float zfar);

void main() {
    vec2  tc = vary_fragcoord.xy;
    vec4 pos = getPositionWithDepth(tc, getDepth(tc));
    frag_color = pos;
}
