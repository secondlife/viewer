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
uniform sampler2D specularRect;
uniform sampler2D sceneMap;
uniform sampler2D diffuseRect;
uniform sampler2D diffuseMap;

vec3 getNorm(vec2 screenpos);
float getDepth(vec2 pos_screen);
float linearDepth(float d, float znear, float zfar);
float linearDepth01(float d, float znear, float zfar);

vec4 getPositionWithDepth(vec2 pos_screen, float depth);
vec4 getPosition(vec2 pos_screen);
vec4 getNormalEnvIntensityFlags(vec2 screenpos, out vec3 n, out float envIntensity);
bool traceScreenRay(vec3 position, vec3 reflection, out vec4 hitColor, out float hitDepth, float depth, sampler2D textureFrame);
float random (vec2 uv);
void main() {
    vec2  tc = vary_fragcoord.xy;
    float depth = linearDepth01(getDepth(tc), zNear, zFar);
    vec3 n = vec3(0, 0, 1);
    float envIntensity;
    vec4 norm = getNormalEnvIntensityFlags(tc, n, envIntensity); // need `norm.w` for GET_GBUFFER_FLAG()
    vec3 pos = getPositionWithDepth(tc, getDepth(tc)).xyz;
    vec4 spec    = texture2D(specularRect, tc);
    vec3 viewPos = camera_ray * depth;
    vec3 rayDirection = normalize(reflect(normalize(viewPos), n)) * -viewPos.z;
    vec2 hitpixel;
    vec4 hitpoint;
    vec4 diffuse = texture2D(diffuseRect, tc);
    vec3 specCol = spec.rgb;

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR)) {
        vec3 orm = specCol.rgb;
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        vec3 f0 = vec3(0.04);
        vec3 baseColor = diffuse.rgb;
        
        vec3 diffuseColor = baseColor.rgb*(vec3(1.0)-f0);

        specCol = mix(f0, baseColor.rgb, metallic);
    }

	vec2 uv2 = tc * screen_res;
	float c = (uv2.x + uv2.y) * 0.125;
	float jitter = mod( c, 1.0);

    vec3 firstBasis = normalize(cross(vec3(1.f, 1.f, 1.f), rayDirection));
	vec3 secondBasis = normalize(cross(rayDirection, firstBasis));
    
    frag_color = texture(diffuseMap, tc);
    vec4 collectedColor;
    
    vec2 screenpos = 1 - abs(tc * 2 - 1);
    float vignette = clamp((screenpos.x * screenpos.y) * 16,0, 1);
    vignette *= clamp((dot(normalize(viewPos), n) * 0.5 + 0.5 - 0.2) * 8, 0, 1);
    vignette *= min(linearDepth(getDepth(tc), zNear, zFar) / zFar, 1);
    int totalSamples = 4;
    for (int i = 0; i < totalSamples; i++) {
		vec2 coeffs = vec2(random(tc + vec2(0, i)) + random(tc + vec2(i, 0)));
		vec3 reflectionDirectionRandomized = rayDirection + firstBasis * coeffs.x + secondBasis * coeffs.y;

        bool hit = traceScreenRay(pos, reflectionDirectionRandomized, hitpoint, depth, depth, diffuseMap);
        if (hit) {
            collectedColor += hitpoint;
            collectedColor.rgb *= specCol.rgb;
        }
	}

    collectedColor *= vignette;

    frag_color += collectedColor;
}
