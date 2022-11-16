/**
 * @file class3/deferred/screenSpaceReflUtil.glsl
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

uniform sampler2D depthMap;
uniform sampler2D normalMap;
uniform sampler2D sceneMap;
uniform vec2 screen_res;
uniform mat4 projection_matrix;
uniform float zNear;
uniform float zFar;
uniform mat4 inv_proj;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);
float linearDepth(float depth, float near, float far);
float getDepth(vec2 pos_screen);
float linearDepth01(float d, float znear, float zfar);

float random (vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123); //simple random function
}

// Based off of https://github.com/RoundedGlint585/ScreenSpaceReflection/
// A few tweaks here and there to suit our needs.

vec2 generateProjectedPosition(vec3 pos){
	vec4 samplePosition = projection_matrix * vec4(pos, 1.f);
	samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;
	return samplePosition.xy;
}

bool isBinarySearchEnabled = true;
bool isAdaptiveStepEnabled = true;
bool isExponentialStepEnabled = true;
bool debugDraw = false;
int iterationCount = 40;
float rayStep = 0.1;
float distanceBias = 0.02;
float depthRejectBias = 0.001;
float epsilon = 0.1;

bool traceScreenRay(vec3 position, vec3 reflection, out vec4 hitColor, out float hitDepth, float depth, sampler2D textureFrame) {
	vec3 step = rayStep * reflection;
	vec3 marchingPosition = position + step;
	float delta;
	float depthFromScreen;
	vec2 screenPosition;
    bool hit = false;
    hitColor = vec4(0);
	
	int i = 0;
	if (depth > depthRejectBias) {
		for (; i < iterationCount && !hit; i++) {
			screenPosition = generateProjectedPosition(marchingPosition);
				depthFromScreen = linearDepth(getDepth(screenPosition), zNear, zFar);
			delta = abs(marchingPosition.z) - depthFromScreen;
			
			if (depth < depthFromScreen + epsilon && depth > depthFromScreen - epsilon) {
				break;
			}

			if (abs(delta) < distanceBias) {
				vec4 color = vec4(1);
				if(debugDraw)
					color = vec4( 0.5+ sign(delta)/2,0.3,0.5- sign(delta)/2, 0);
				hitColor = texture(textureFrame, screenPosition) * color;
				hitDepth = depthFromScreen;
				hit = true;
				break;
			}
			if (isBinarySearchEnabled && delta > 0) {
				break;
			}
			if (isAdaptiveStepEnabled){
				float directionSign = sign(abs(marchingPosition.z) - depthFromScreen);
				//this is sort of adapting step, should prevent lining reflection by doing sort of iterative converging
				//some implementation doing it by binary search, but I found this idea more cheaty and way easier to implement
				step = step * (1.0 - rayStep * max(directionSign, 0.0));
				marchingPosition += step * (-directionSign);
			}
			else {
				marchingPosition += step;
			}

			if (isExponentialStepEnabled){
				step *= 1.05;
			}
		}
		if(isBinarySearchEnabled){
			for(; i < iterationCount && !hit; i++){
			
				step *= 0.5;
				marchingPosition = marchingPosition - step * sign(delta);
				
				screenPosition = generateProjectedPosition(marchingPosition);
				depthFromScreen = linearDepth(getDepth(screenPosition), zNear, zFar);
				delta = abs(marchingPosition.z) - depthFromScreen;

				if (depth < depthFromScreen + epsilon && depth > depthFromScreen - epsilon) {
					break;
				}

				if (abs(delta) < distanceBias && depthFromScreen != (depth - distanceBias)) {
					vec4 color = vec4(1);
					if(debugDraw)
						color = vec4( 0.5+ sign(delta)/2,0.3,0.5- sign(delta)/2, 0);
					hitColor = texture(textureFrame, screenPosition) * color;
					hitDepth = depthFromScreen;
					hit = true;
					break;
				}
			}
		}
	}
	
    return hit;
}
