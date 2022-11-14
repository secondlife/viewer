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
VARYING vec3 camera_ray;

uniform sampler2D depthMap;
uniform sampler2D normalMap;
uniform sampler2D sceneMap;
uniform sampler2D diffuseRect;

vec3 getNorm(vec2 screenpos);
float getDepth(vec2 pos_screen);
float linearDepth(float d, float znear, float zfar);
float linearDepth01(float d, float znear, float zfar);

vec4 getPositionWithDepth(vec2 pos_screen, float depth);
vec4 getPosition(vec2 pos_screen);

bool traceScreenRay(vec3 position, vec3 reflection, out vec3 hitColor, out float hitDepth, float depth, sampler2D textureFrame);
float random (vec2 uv);
void main() {
    vec2  tc = vary_fragcoord.xy;
    float depth = linearDepth01(getDepth(tc), zNear, zFar);
    vec3 pos = getPositionWithDepth(tc, getDepth(tc)).xyz;
    vec3 viewPos = camera_ray * depth;
    vec3 rayDirection = normalize(reflect(normalize(viewPos), getNorm(tc))) * -viewPos.z;
    vec2 hitpixel;
    vec3 hitpoint;

	vec2 uv2 = tc * screen_res;
	float c = (uv2.x + uv2.y) * 0.125;
	float jitter = mod( c, 1.0);

    vec3 firstBasis = normalize(cross(vec3(0.f, 0.f, 1.f), rayDirection));
	vec3 secondBasis = normalize(cross(rayDirection, firstBasis));
    
    frag_color.rgb = texture(diffuseRect, tc).rgb;
    vec4 collectedColor;
    for (int i = 0; i < 1; i++) {
		vec2 coeffs = vec2(random(tc + vec2(0, i)) + random(tc + vec2(i, 0))) * 0.25;
		vec3 reflectionDirectionRandomized = rayDirection + firstBasis * coeffs.x + secondBasis * coeffs.y;

        bool hit = traceScreenRay(pos, reflectionDirectionRandomized, hitpoint, depth, depth, diffuseRect);
        if (hit) {
            vec2 screenpos = tc * 2 - 1;
            float vignette = 1;// clamp((1 - dot(screenpos, screenpos)) * 4,0, 1);
            vignette *= dot(normalize(viewPos), getNorm(tc)) * 0.5 + 0.5;
            vignette *= min(linearDepth(getDepth(tc), zNear, zFar) / (zFar * 0.0125), 1);
            collectedColor.rgb = hitpoint * vignette * 0.25;
            frag_color.rgb = hitpoint;
        }
	}

    //frag_color.rgb = collectedColor.rgb;



    frag_color.a = 1.0;
}
